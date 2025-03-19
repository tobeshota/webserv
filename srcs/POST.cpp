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
      _rootDirective.findDirective(_httpRequest.getHeader("Host"));
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
  return access(path.c_str(), W_OK) == 0;
}

// リクエストボディサイズが制限内か確認する関数
bool POST::isBodySizeAllowed(const std::string& body) const {
  const Directive* hostDirective = 
      _rootDirective.findDirective(_httpRequest.getHeader("Host"));
  
  if (hostDirective != NULL) {
    std::string maxBodySizeStr = hostDirective->getValue("client_max_body_size");
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
  // URLからパスを取得
  std::string url = _httpRequest.getURL();
  
  // ディレクトリの場合は末尾に/を保証
  std::string locationPath = url;
  if (!locationPath.empty() && locationPath[locationPath.length() - 1] != '/') {
    // 親ディレクトリを取得
    size_t pos = locationPath.find_last_of('/');
    if (pos != std::string::npos) {
      locationPath = locationPath.substr(0, pos + 1);
    }
  }
  
  // locationディレクティブを探す
  const Directive* locationDirective = 
      _rootDirective.findDirective(_httpRequest.getHeader("Host"), "location", locationPath);
  
  if (locationDirective != NULL) {
    // limit_except ディレクティブがあるかチェック
    std::string allowedMethods = locationDirective->getValue("limit_except");
    if (!allowedMethods.empty()) {
      // POSTメソッドが許可されているか確認
      return allowedMethods.find("POST") != std::string::npos;
    } else {
      // limit_exceptディレクティブがない場合はデフォルトで許可
      return true;
    }
  }
  
  // ディレクティブが見つからない場合はデフォルトで許可
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
bool POST::writeToFile(const std::string& filePath, const std::string& content) {
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
  }

  if (!directoryExists(dirPath)) {
    httpResponse.setHttpStatusCode(404);  // Not Found
    return;
  }

  // 書き込み権限の確認
  if (!hasWritePermission(dirPath)) {
    httpResponse.setHttpStatusCode(403);  // Forbidden
    return;
  }

  // デフォルトは200 OK (後でhandlePostRequestで201に変更される可能性あり)
  httpResponse.setHttpStatusCode(200);
}

// POSTリクエストを処理する関数
bool POST::handlePostRequest(HTTPResponse& httpResponse, const std::string& fullPath) {
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
  
  // ステータスコードが200ならPOST処理を実行
  if (httpResponse.getHttpStatusCode() == 200) {
    handlePostRequest(httpResponse, fullPath);
  }
  
  // チェーンの次のハンドラに処理を委譲
  if (_nextHandler != NULL) {
    _nextHandler->handleRequest(httpResponse);
  }
}
