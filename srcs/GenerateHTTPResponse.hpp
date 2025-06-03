#pragma once

#include <sys/stat.h>  // for stat

#include <fstream>  // ファイル入出力を行うためのヘッダ。std::ifstreamやstd::ofstreamなどのファイル入出力ストリームを提供。
#include <sstream>  // 文字列のストリーム操作を行うためのヘッダ。std::stringstreamを使って文字列を組み立て、最終的に出力するために使用。

#include "CGI.hpp"
#include "Handler.hpp"
#include "ListenDirectory.hpp"
#include "StatusCodes.hpp"
#define DEFAULT_ERROR_PAGE "html/defaultErrorPage.html"

class GenerateHTTPResponse : public Handler {
 private:
  Directive _rootDirective;
  HTTPRequest _httpRequest;
  std::string generateHttpStatusLine(const int status_code);
  std::string generateHttpResponseHeader(const std::string& httpResponseBody);
  std::string generateHttpResponseBody(const int status_code);
  // utils
  std::string getSuccessPathForHttpResponseBody();
  std::string getErrorPathForHttpResponseBody(const int status_code);

  // MIME タイプ関連のメソッドを追加
  std::string getMimeType(const std::string& filePath);
  std::string getFileExtension(const std::string& filePath);

  // ファイル存在チェック関数を追加
  bool fileExists(const std::string& filePath);

 public:
  GenerateHTTPResponse(Directive rootDirective, HTTPRequest httpRequest);
  std::string getDirectiveValue(std::string directiveKey);
  std::vector<std::string> getDirectiveValues(std::string directiveKey);

  void handleRequest(HTTPResponse& httpResponse);
};
