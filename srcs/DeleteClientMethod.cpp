#include "DeleteClientMethod.hpp"
#include <cstdio> // std::removeのため

void DeleteClientMethod::handleRequest(HTTPResponse& httpResponse) {
  // URLからファイルパスを取得
  std::string filePath = _http.getURL();
  
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
