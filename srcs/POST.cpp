#include "POST.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

POST::POST(Directive rootDirective, HTTPRequest httpRequest)
    : _rootDirective(rootDirective), _httpRequest(httpRequest) {}

// 完全なファイルパスを取得する関数
std::string POST::getFullPath() const {
  // ホストディレクティブからrootの値を取得
  std::string rootValue;
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getServerName());
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  // URLとrootを結合して完全なパスを作成
  return rootValue + _httpRequest.getURL();
}

// ファイルが存在するか確認する関数
bool POST::fileExists(const std::string& filePath) const {
  struct stat buffer;
  return (stat(filePath.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

// ディレクトリが存在するか確認する関数
bool POST::directoryExists(const std::string& dirPath) const {
  struct stat buffer;
  return (stat(dirPath.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

// ファイルまたはディレクトリへの書き込み権限があるか確認する関数
bool POST::hasWritePermission(const std::string& path) const {
  struct stat buffer;

  // ファイルまたはディレクトリが存在するか確認
  if (stat(path.c_str(), &buffer) != 0) {
    return false;  // 存在しない場合は書き込み不可
  }

  // ディレクトリの場合は、より厳密にパーミッションビットをチェック
  if (S_ISDIR(buffer.st_mode)) {
    // ディレクトリの書き込み権限ビットをチェック
    mode_t mode = buffer.st_mode;
    // 実行ユーザーが所有者の場合
    if (buffer.st_uid == getuid()) {
      return (mode & S_IWUSR) != 0;
    }
    // 実行ユーザーがグループメンバーの場合
    else if (buffer.st_gid == getgid()) {
      return (mode & S_IWGRP) != 0;
    }
    // その他のユーザーの場合
    else {
      return (mode & S_IWOTH) != 0;
    }
  }

  // 実際のアクセス権限をチェック（ファイルの場合）
  return access(path.c_str(), W_OK) == 0;
}

// リクエストボディサイズが制限内か確認する関数
bool POST::isBodySizeAllowed(const std::string& body) const {
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getServerName());

  if (hostDirective != NULL) {
    std::string maxBodySizeStr =
        hostDirective->getValue("client_max_body_size");
    if (!maxBodySizeStr.empty()) {
      // クライアント最大ボディサイズを解析
      size_t maxBodySize = 0;
      std::istringstream iss(maxBodySizeStr);
      iss >> maxBodySize;

      // サイズ単位（M, K, G）があれば考慮
      if (iss.peek() == 'M' || iss.peek() == 'm') {
        maxBodySize *= 1024 * 1024;  // メガバイト
      } else if (iss.peek() == 'K' || iss.peek() == 'k') {
        maxBodySize *= 1024;  // キロバイト
      } else if (iss.peek() == 'G' || iss.peek() == 'g') {
        maxBodySize *= 1024 * 1024 * 1024;  // ギガバイト
      }

      return body.size() <= maxBodySize;
    }
  }

  // 制限が指定されていない場合はデフォルトで1MBに制限
  return body.size() <= 1024 * 1024;
}

// 指定されたディレクトリにPOSTが許可されているか確認する関数
bool POST::isPostAllowedForPath(const std::string& path) const {
  // 未使用パラメータの警告を抑制
  (void)path;

  // URLからパスを取得
  std::string url = _httpRequest.getURL();

  // ホストディレクティブを取得
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getServerName());

  if (hostDirective == NULL) {
    return true;  // ホストディレクティブが見つからなければデフォルトで許可
  }

  // 最適なlocationディレクティブを探す
  std::string bestMatch = "";
  const Directive* bestLocationDirective = NULL;

  // ホストディレクティブの子ディレクティブを探索
  const Directive::DirectiveList& children = hostDirective->children();
  for (size_t i = 0; i < children.size(); i++) {
    if (children[i].name() == "location") {
      // locationディレクティブからパス値を取得
      std::string locationPath = "";
      Directive::KVMap::const_iterator it =
          children[i].keyValues().find("path");
      if (it != children[i].keyValues().end() && !it->second.empty()) {
        locationPath = it->second[0];
      }

      // URLがlocationPathで始まるかチェック
      if (url.find(locationPath) == 0) {
        // より長いマッチを優先 (より具体的なlocation)
        if (locationPath.length() > bestMatch.length()) {
          bestMatch = locationPath;
          bestLocationDirective = &children[i];
        }
      }
    }
  }

  if (bestLocationDirective != NULL) {
    // limit_except ディレクティブの値を確認
    std::string allowedMethods =
        bestLocationDirective->getValue("limit_except");
    if (!allowedMethods.empty()) {
      // POSTメソッドが許可されているか確認
      return allowedMethods.find("POST") != std::string::npos;
    }
  }

  // マッチするlocationディレクティブが見つからない場合はデフォルトで許可
  return true;
}

// チャンク転送エンコーディングを処理する関数
std::string POST::processChunkedBody(const std::string& chunkedBody) {
  std::string result;
  size_t pos = 0;

  while (pos < chunkedBody.size()) {
    // チャンクサイズを読み取る
    size_t eol = chunkedBody.find("\r\n", pos);
    if (eol == std::string::npos) break;

    std::string chunkSizeHex = chunkedBody.substr(pos, eol - pos);
    // 16進数サイズを取得
    size_t chunkSize = 0;
    std::istringstream iss(chunkSizeHex);
    iss >> std::hex >> chunkSize;

    // サイズが0ならチャンク終了
    if (chunkSize == 0) break;

    // チャンクデータの開始位置
    size_t dataStart = eol + 2;
    if (dataStart + chunkSize <= chunkedBody.size()) {
      // チャンクデータをresultに追加
      result.append(chunkedBody, dataStart, chunkSize);
      // 次のチャンクへ
      pos = dataStart + chunkSize + 2;  // +2 for \r\n after chunk data
    } else {
      // チャンクデータが不完全
      break;
    }
  }

  return result;
}

// ファイルにデータを書き込む関数
bool POST::writeToFile(const std::string& filePath,
                       const std::string& content) {
  // ファイルを開く（既存ファイルは上書き）
  std::ofstream file(filePath.c_str(), std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  // コンテンツを書き込む
  file.write(content.c_str(), content.size());
  bool success = !file.bad();
  file.close();

  return success;
}

// HTTPステータスコードを設定する関数
void POST::setHttpStatusCode(HTTPResponse& httpResponse,
                             const std::string& fullPath) {
  std::string method = _httpRequest.getMethod();

  // メソッド確認 (POSTでないなら405)
  if (method != "POST") {
    httpResponse.setHttpStatusCode(405);  // Method Not Allowed
    return;
  }

  // POSTメソッドが許可されているか確認
  if (!isPostAllowedForPath(fullPath)) {
    httpResponse.setHttpStatusCode(405);  // Method Not Allowed
    return;
  }

  // リクエストボディのサイズ確認
  std::string body = _httpRequest.getBody();

  // チャンク転送の場合はデコード
  if (_httpRequest.getHeader("Transfer-Encoding") == "chunked") {
    body = processChunkedBody(body);
  }

  // ボディサイズ制限チェック
  if (!isBodySizeAllowed(body)) {
    httpResponse.setHttpStatusCode(413);  // Request Entity Too Large
    return;
  }

  // ファイル/ディレクトリの存在確認
  std::string dirPath = fullPath;
  size_t lastSlash = dirPath.find_last_of('/');
  if (lastSlash != std::string::npos) {
    dirPath = dirPath.substr(0, lastSlash);
  } else {
    // パスにスラッシュがない場合はエラー
    httpResponse.setHttpStatusCode(500);  // Internal Server Error
    return;
  }

  // ディレクトリの存在確認
  if (!directoryExists(dirPath)) {
    httpResponse.setHttpStatusCode(404);  // Not Found
    return;
  }

  // ディレクトリの書き込み権限確認 - この確認を厳密に行う
  if (!hasWritePermission(dirPath)) {
    httpResponse.setHttpStatusCode(403);  // Forbidden
    return;
  }

  // ファイルが既に存在する場合は、ファイルの書き込み権限も確認
  if (fileExists(fullPath) && !hasWritePermission(fullPath)) {
    httpResponse.setHttpStatusCode(403);  // Forbidden
    return;
  }

  // デフォルトは200 OK (後でhandlePostRequestで201に変更される可能性あり)
  httpResponse.setHttpStatusCode(200);
}

// マルチパートフォームデータかどうか確認する関数
bool POST::isMultipartForm() const {
  std::string contentType = _httpRequest.getHeader("Content-Type");
  return contentType.find("multipart/form-data") != std::string::npos;
}

// Content-Typeヘッダーから境界区切り文字を抽出する関数
std::string POST::extractBoundary() const {
  std::string contentType = _httpRequest.getHeader("Content-Type");
  size_t boundaryPos = contentType.find("boundary=");

  if (boundaryPos == std::string::npos) {
    return "";
  }

  // "boundary=" の後の文字列を抽出
  std::string boundary = contentType.substr(boundaryPos + 9);
  // 余分なパラメータがある場合は除去
  size_t endPos = boundary.find(";");
  if (endPos != std::string::npos) {
    boundary = boundary.substr(0, endPos);
  }

  return boundary;
}

// マルチパートフォームデータを解析する関数
std::vector<POST::MultipartData> POST::parseMultipartFormData(
    const std::string& body, const std::string& boundary) {
  std::vector<MultipartData> result;

  // 境界文字列の形式
  std::string delimiter = "--" + boundary;
  std::string endDelimiter = delimiter + "--";

  size_t pos = body.find(delimiter);
  if (pos == std::string::npos) {
    return result;
  }

  while (pos != std::string::npos) {
    // 次の境界位置を検索
    size_t nextPos = body.find(delimiter, pos + delimiter.length());
    if (nextPos == std::string::npos) {
      // 終端の境界を確認
      nextPos = body.find(endDelimiter, pos + delimiter.length());
      if (nextPos == std::string::npos) {
        break;
      }
    }

    // 現在のパート部分を取得
    std::string part = body.substr(pos + delimiter.length(),
                                   nextPos - pos - delimiter.length());

    // ヘッダーとコンテンツの区切り（空行）を検索
    size_t headerEnd = part.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
      pos = nextPos;
      continue;
    }

    // ヘッダー部分とコンテンツ部分に分離
    std::string headers = part.substr(0, headerEnd);
    std::string content = part.substr(headerEnd + 4);  // 4は\r\n\r\nの長さ

    // 末尾の\r\nを削除
    if (content.length() >= 2 &&
        content.substr(content.length() - 2) == "\r\n") {
      content = content.substr(0, content.length() - 2);
    }

    // Content-Dispositionヘッダーを解析
    size_t cdPos = headers.find("Content-Disposition:");
    if (cdPos != std::string::npos) {
      // 行末までを取得
      size_t cdEndPos = headers.find("\r\n", cdPos);
      if (cdEndPos == std::string::npos) {
        cdEndPos = headers.length();
      }

      std::string cdValue = headers.substr(cdPos, cdEndPos - cdPos);

      // フィールド名を取得
      std::string fieldName = "";
      size_t namePos = cdValue.find("name=\"");
      if (namePos != std::string::npos) {
        size_t nameEndPos = cdValue.find("\"", namePos + 6);
        if (nameEndPos != std::string::npos) {
          fieldName = cdValue.substr(namePos + 6, nameEndPos - namePos - 6);
        }
      }

      // ファイル名を取得
      std::string fileName = "";
      size_t fileNamePos = cdValue.find("filename=\"");
      if (fileNamePos != std::string::npos) {
        size_t fileNameEndPos = cdValue.find("\"", fileNamePos + 10);
        if (fileNameEndPos != std::string::npos) {
          fileName = cdValue.substr(fileNamePos + 10,
                                    fileNameEndPos - fileNamePos - 10);
        }
      }

      // マルチパートデータ構造体を作成して結果に追加
      MultipartData data;
      data.fieldName = fieldName;
      data.fileName = fileName;
      data.content = content;
      result.push_back(data);
    }

    pos = nextPos;
  }

  return result;
}

// マルチパートフォームを処理する関数
bool POST::handleMultipartForm(HTTPResponse& httpResponse,
                               const std::string& dirPath) {
  std::string body = _httpRequest.getBody();
  std::string boundary = extractBoundary();

  if (boundary.empty()) {
    httpResponse.setHttpStatusCode(400);  // Bad Request
    return false;
  }

  std::vector<MultipartData> parts = parseMultipartFormData(body, boundary);

  if (parts.empty()) {
    httpResponse.setHttpStatusCode(400);  // Bad Request
    return false;
  }

  bool success = false;

  // 各パートを処理
  for (size_t i = 0; i < parts.size(); i++) {
    // ファイルアップロードパートの処理
    if (!parts[i].fileName.empty()) {
      // リクエストURLからディレクトリパスを取得
      std::string filePath = dirPath + "/" + parts[i].fileName;

      // ファイルを保存
      if (writeToFile(filePath, parts[i].content)) {
        success = true;
      }
    }
  }

  if (success) {
    httpResponse.setHttpStatusCode(201);  // Created
    return true;
  } else {
    httpResponse.setHttpStatusCode(500);  // Internal Server Error
    return false;
  }
}

// POSTリクエストを処理する関数
bool POST::handlePostRequest(HTTPResponse& httpResponse,
                             const std::string& fullPath) {
  // すでに403などエラーステータスが設定されている場合は処理をスキップ
  if (httpResponse.getHttpStatusCode() != 200) {
    return false;
  }

  // ディレクトリパスを取得
  std::string dirPath = fullPath;
  size_t lastSlash = dirPath.find_last_of('/');
  if (lastSlash != std::string::npos) {
    dirPath = dirPath.substr(0, lastSlash);
  }

  // マルチパートフォームデータの処理
  if (isMultipartForm()) {
    return handleMultipartForm(httpResponse, dirPath);
  }

  // 通常のPOST処理（既存のコード）
  std::string body = _httpRequest.getBody();

  // チャンク転送の場合はデコード
  if (_httpRequest.getHeader("Transfer-Encoding") == "chunked") {
    body = processChunkedBody(body);
  }

  // CGIスクリプトかどうか確認
  if (fullPath.find(".py") != std::string::npos ||
      fullPath.find(".sh") != std::string::npos) {
    // CGIハンドラを呼び出す
    CGI cgi = CGI(_rootDirective, _httpRequest);
    cgi.handleRequest(httpResponse);
    return true;
  } else {
    // 通常のファイル書き込み
    if (writeToFile(fullPath, body)) {
      httpResponse.setHttpStatusCode(201);  // Created
      return true;
    } else {
      httpResponse.setHttpStatusCode(500);  // Internal Server Error
      return false;
    }
  }
}

// HTTPレスポンスを処理する
void POST::handleRequest(HTTPResponse& httpResponse) {
  std::string fullPath = getFullPath();

  // HTTPステータスコードを設定
  setHttpStatusCode(httpResponse, fullPath);

  // ステータスコードが200の場合のみPOST処理を実行
  if (httpResponse.getHttpStatusCode() == 200) {
    handlePostRequest(httpResponse, fullPath);
  }

  // チェーンの次のハンドラに処理を委譲
  if (_nextHandler != NULL) {
    _nextHandler->handleRequest(httpResponse);
  }
}
