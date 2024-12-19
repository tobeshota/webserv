#pragma once

#include "ParseConf.hpp"
#include "OSInit.hpp"
#include "ParseHTTPRequest.hpp"
#include "GenerateHTTPResponse.hpp"
#include <iostream>
// ソースコードが存在しない際に，単体テストの分岐網羅率を100%にするためのコード．
// いずれ削除する
namespace ft {

int abs(int a);
}

int webserv(int argc, char **argv);
