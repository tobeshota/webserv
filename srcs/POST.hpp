#pragma once

#include "CGI.hpp"
#include "GenerateHTTPResponse.hpp"
#include "Handler.hpp"

class POST : public Handler {
 private:
  Directive _rootDirective;
  HTTPRequest _httpRequest;

  // リクエストされたURLの完全なファイルパスを取得する関数
  std::string getFullPath() const;

  // HTTPステータスコードを設定する関数
  void setHttpStatusCode(HTTPResponse& httpResponse,
                         const std::string& fullPath);

  // POSTリクエストを処理する関数
  bool handlePostRequest(HTTPResponse& httpResponse,
                         const std::string& fullPath);

  // ファイルにデータを書き込む関数
  bool writeToFile(const std::string& filePath, const std::string& content);

  // チャンク転送エンコーディングを処理する関数
  std::string processChunkedBody(const std::string& chunkedBody);

  // ファイルが存在するか確認する関数
  bool fileExists(const std::string& filePath) const;

  // ディレクトリが存在するか確認する関数
  bool directoryExists(const std::string& dirPath) const;

  // ファイルまたはディレクトリへの書き込み権限があるか確認する関数
  bool hasWritePermission(const std::string& path) const;

  // リクエストボディサイズが制限内か確認する関数
  bool isBodySizeAllowed(const std::string& body) const;

  // 指定されたディレクトリにPOSTが許可されているか確認する関数
  bool isPostAllowedForPath(const std::string& path) const;

 public:
  POST(Directive rootDirective, HTTPRequest httpRequest);
  void handleRequest(HTTPResponse& httpResponse);
};
