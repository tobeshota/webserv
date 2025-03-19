#pragma once

#include "CGI.hpp"  //  後に実装される
#include "GenerateHTTPResponse.hpp"
#include "Handler.hpp"

// リクエストボディの最大サイズ（例: 10MB）
#define MAX_BODY_SIZE 10485760

class ADD : public Handler {
 private:
  Directive _rootDirective;
  HTTPRequest _httpRequest;

  // リクエストされたURLの完全なファイルパスを取得する関数
  std::string getFullPath() const;

  // HTTPステータスコードを設定する関数
  void setHttpStatusCode(HTTPResponse& httpResponse, const std::string& fullPath);

 public:
  ADD(Directive rootDirective, HTTPRequest httpRequest);
  void handleRequest(HTTPResponse& httpResponse);
};
