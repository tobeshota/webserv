#include "DeleteClientMethod.hpp"

#include <cstdio>  // std::removeのため

// 完全なファイルパスを取得する関数
std::string DeleteClientMethod::getFullPath() const {
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

void DeleteClientMethod::handleRequest(HTTPResponse& httpResponse) {
  // URLからファイルパスを取得
  std::string filePath = getFullPath();

  // ファイルを削除
  if (filePath.empty() || std::remove(filePath.c_str()) != 0) {
    // 削除に失敗した場合（ファイルが存在しない場合も含む）
    httpResponse.setHttpStatusCode(404);
  } else {
    // 削除に成功した場合
    httpResponse.setHttpStatusCode(200);
  }

  if (_nextHandler != NULL) {
    _nextHandler->handleRequest(httpResponse);
  }
}
