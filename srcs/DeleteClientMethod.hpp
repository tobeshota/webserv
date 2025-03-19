#pragma once

#include "Directive.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Handler.hpp"

class DeleteClientMethod : public Handler {
 public:
  // ファイルの状態をチェックするための列挙型
  enum FileStatus { 
    FILE_OK,            // ファイルが存在し、削除可能
    FILE_NOT_FOUND,     // ファイルが存在しない
    FILE_NO_PERMISSION, // 権限がない
    FILE_IS_DIRECTORY,  // ディレクトリである
    FILE_OTHER_ERROR    // その他のエラー
  };
  
 protected:
  HTTPRequest _httpRequest;
  Directive _rootDirective;

 public:
  DeleteClientMethod(HTTPRequest& _httpRequest, Directive rootDirective) : Handler() {
    this->_httpRequest = _httpRequest;
    this->_rootDirective = rootDirective;
  }
  ~DeleteClientMethod() {}
  
  // ファイルパスを取得するメソッド
  std::string getFullPath() const;
  
  // ファイルの状態をチェックするメソッド
  FileStatus checkFileStatus(const std::string& filePath) const;
  
  // HTTPステータスコードを決定するメソッド
  int determineStatusCode(const std::string& filePath) const;
  
  // リクエストハンドラ
  void handleRequest(HTTPResponse& httpResponse);
};
