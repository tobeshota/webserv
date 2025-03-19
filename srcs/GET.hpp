#pragma once

#include "CGI.hpp"
#include "GenerateHTTPResponse.hpp"
#include "Handler.hpp"

class GET : public Handler {
 private:
  Directive _rootDirective;
  HTTPRequest _httpRequest;

  // リクエストされたURLの完全なファイルパスを取得する関数
  std::string getFullPath() const;

  // HTTPステータスコードを設定する関数
  void setHttpStatusCode(HTTPResponse& httpResponse,
                         const std::string& fullPath);

 public:
  GET(Directive rootDirective, HTTPRequest httpRequest);
  void handleRequest(HTTPResponse& httpResponse);
};
