#include "DeleteClientMethod.hpp"

#include <sys/stat.h>  // stat関数用

#include <cstdio>   // std::removeのため
#include <fstream>  // ファイル存在チェック

#include "GenerateHTTPResponse.hpp"  //  for getDirectiveValues

// 完全なファイルパスを取得する関数
std::string DeleteClientMethod::getFullPath() const {
  // URLが空の場合は早期リターン
  if (_httpRequest.getURL().empty()) {
    return "";
  }

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

// ファイルの状態をチェックするメソッド
DeleteClientMethod::FileStatus DeleteClientMethod::checkFileStatus(
    const std::string& filePath) const {
  if (filePath.empty()) {
    return FILE_NOT_FOUND;
  }

  // statを使ってファイル情報を取得
  struct stat fileInfo;
  if (stat(filePath.c_str(), &fileInfo) != 0) {
    return FILE_NOT_FOUND;
  }

  // ディレクトリかどうかチェック
  if (S_ISDIR(fileInfo.st_mode)) {
    return FILE_IS_DIRECTORY;
  }

  // 書き込み権限チェック
  std::fstream testAccess;
  testAccess.open(filePath.c_str(), std::ios::in | std::ios::out);
  if (!testAccess) {
    return FILE_NO_PERMISSION;
  }
  testAccess.close();

  return FILE_OK;
}

// HTTPステータスコードを決定するメソッド
int DeleteClientMethod::determineStatusCode(const std::string& filePath) const {
  FileStatus status = checkFileStatus(filePath);

  switch (status) {
    case FILE_OK:
      return 204;  // No Content - 削除成功（内容は返さない）
    case FILE_NOT_FOUND:
      return 404;  // Not Found
    case FILE_NO_PERMISSION:
      return 403;  // Forbidden
    case FILE_IS_DIRECTORY:
      return 405;  // Method Not Allowed（ディレクトリ削除は許可しない）
    case FILE_OTHER_ERROR:
    default:
      return 500;  // Internal Server Error
  }
}

static bool isContain(std::vector<std::string> vec, std::string str) {
  for (std::vector<std::string>::iterator itr = vec.begin(); itr != vec.end();
       ++itr) {
    if (*itr == str) {
      return true;
    }
  }
  return false;
}

void DeleteClientMethod::handleRequest(HTTPResponse& httpResponse) {
  GenerateHTTPResponse generateHTTPResponse(_rootDirective, _httpRequest);

  if (isContain(generateHTTPResponse.getDirectiveValues("deny"),
                _httpRequest.getMethod())) {
    httpResponse.setHttpStatusCode(405);
    if (_nextHandler != NULL) {
      _nextHandler->handleRequest(httpResponse);
    }
    return;
  }

  // URLが空かどうかを明示的にチェック
  if (_httpRequest.getURL().empty()) {
    httpResponse.setHttpStatusCode(404);  // Not Found
    if (_nextHandler != NULL) {
      _nextHandler->handleRequest(httpResponse);
    }
    return;
  }

  // URLからファイルパスを取得
  std::string filePath = getFullPath();

  // ファイルパスが空かどうかをチェック（root値が取得できない場合など）
  if (filePath.empty()) {
    httpResponse.setHttpStatusCode(404);  // Not Found
    if (_nextHandler != NULL) {
      _nextHandler->handleRequest(httpResponse);
    }
    return;
  }

  // ファイルの状態をチェック
  FileStatus status = checkFileStatus(filePath);

  if (status == FILE_OK) {
    // ファイル削除を試みる
    if (std::remove(filePath.c_str()) == 0) {
      // 削除に成功
      httpResponse.setHttpStatusCode(204);  // No Content
    } else {
      // 削除中に問題が発生
      httpResponse.setHttpStatusCode(500);  // Internal Server Error
    }
  } else {
    // 状態に応じたステータスコードを設定
    httpResponse.setHttpStatusCode(determineStatusCode(filePath));
  }

  // チェーン内の次のハンドラーがあれば呼び出す
  if (_nextHandler != NULL) {
    _nextHandler->handleRequest(httpResponse);
  }
}
