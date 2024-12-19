#pragma once

#include <iostream>

#include "GenerateHTTPResponse.hpp"
#include "OSInit.hpp"
#include "ParseConf.hpp"
#include "ParseHTTPRequest.hpp"

int webserv(int argc, char **argv);

// ソースコードが存在しない際に，単体テストの分岐網羅率を100%にするためのコード．
// いずれ削除する
namespace ft {
int abs(int n);
}  // namespace ft
