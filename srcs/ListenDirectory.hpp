#pragma once

#include <string>
#include <vector>

#include "Handler.hpp"

// 前方宣言
std::string int2str(size_t value);

class ListenDirectory : public Handler {
 private:
  std::string _dirpath;  // ディレクトリパスを保持するメンバ変数

 public:
  ListenDirectory(const std::string& dirpath);
  void handleRequest(HTTPResponse& httpResponse);
};
