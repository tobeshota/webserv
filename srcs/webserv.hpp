#pragma once

#include <iostream>

#include "GenerateHTTPResponse.hpp"
#include "OSInit.hpp"
#include "ParseConf.hpp"
#include "ParseHTTPRequest.hpp"
#include "Run_Server.hpp"
#include "ServerData.hpp"

int webserv(int argc, char **argv);

namespace utilities {
int check_func(int func, std::string error_message);
}

// ソースコードが存在しない際に，単体テストの分岐網羅率を100%にするためのコード．
// いずれ削除する
namespace ft {
int abs(int n);
}  // namespace ft
