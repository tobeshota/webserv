#include "GET.hpp"

#include <sys/stat.h>
#include <unistd.h>

GET::GET(Directive rootDirective, HTTPRequest httpRequest)
    : _rootDirective(rootDirective), _httpRequest(httpRequest) {}

// 指定された文字列が任意の文字列で終わるかを調べる関数
static bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ファイルが存在するか確認する関数
static bool fileExists(const std::string& filePath) {
  struct stat buffer;
  return (stat(filePath.c_str(), &buffer) == 0);
}

// ファイルが読み取り可能か確認する関数
static bool fileIsReadable(const std::string& filePath) {
  return access(filePath.c_str(), R_OK) == 0;
}

// 完全なファイルパスを取得する関数
std::string GET::getFullPath() const {
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

// HTTPステータスコードを設定する関数
void GET::setHttpStatusCode(HTTPResponse& httpResponse,
                            const std::string& fullPath) {
  std::string method = _httpRequest.getMethod();

  // メソッド確認
  if (method != "GET" && method != "POST" && method != "DELETE") {
    httpResponse.setHttpStatusCode(405);  // Method Not Allowed
  }
  // ファイルの存在確認
  else if (!fileExists(fullPath)) {
    httpResponse.setHttpStatusCode(404);  // Not Found
  }
  // ファイルへのアクセス権の確認
  else if (!fileIsReadable(fullPath)) {
    httpResponse.setHttpStatusCode(403);  // Forbidden
  }
  // 上記の全ての条件を満たしていれば200 OK
  else {
    httpResponse.setHttpStatusCode(200);  // OK
  }
}

// HTTPレスポンスがCGIか確認する
void GET::handleRequest(HTTPResponse& httpResponse) {
  // CGIの場合はCGIハンドラを呼び出す
  if (endsWith(_httpRequest.getURL(), ".py") ||
      endsWith(_httpRequest.getURL(), ".sh")) {
    CGI cgi = CGI(_rootDirective, _httpRequest);
    cgi.handleRequest(httpResponse);
    // CGIハンドラがステータスコードを設定している前提
  } else {
    // 通常のファイルリクエストの場合
    std::string fullPath = getFullPath();
    setHttpStatusCode(httpResponse, fullPath);
  }

  if (_nextHandler != NULL) _nextHandler->handleRequest(httpResponse);
}
