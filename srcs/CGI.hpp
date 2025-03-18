#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "Directive.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Handler.hpp"

// CGIの応答を保存するファイルのパス
#define CGI_PAGE "/tmp/.cgi_response.html"
// CGI実行のタイムアウト（秒）
#define CGI_TIMEOUT 30

class CGI : public Handler {
 private:
  Directive _rootDirective;
  HTTPRequest _httpRequest;

  // CGIスクリプトを実行するためのメソッド
  bool executeCGI(const std::string& scriptPath);

  // 環境変数を設定するメソッド
  char** setupEnvironment(const std::string& scriptPath);

  // 結果を読み込むメソッド
  bool readCGIResponse();

  // ユーティリティメソッド
  bool isPythonScript(const std::string& url) const;
  std::string getScriptPath() const;
  void cleanupEnv(char** env) const;

  // ディレクトリインデックスを生成するメソッドを追加
  std::string generateDirectoryListing(const std::string& dirPath) const;

 public:
  CGI(Directive rootDirective, HTTPRequest httpRequest);
  ~CGI();

  void handleRequest(HTTPResponse& httpResponse);
};
