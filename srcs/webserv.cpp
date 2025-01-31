#include <iostream>

#include "GenerateHTTPResponse.hpp"
#include "OSInit.hpp"
#include "ParseConf.hpp"
#include "ParseHTTPRequest.hpp"

int webserv(int argc, char **argv) {
  // GenerateHTTPResponse generate_http_response;
  (void)argc;
  (void)argv;

  // Confクラス（webserv.confの設定まとめ）を生成する
  // ParseConf();
  // サーバーを構築する
  // OSInit();
  // while (1) {
  // HTTPリクエストをパースする
  // const HTTPRequest http_request = ParseHTTPRequest();
  // HTTPリクエストからHTTPレスポンスを生成する
  // generate_http_response.handleRequest(http_request);
  // 	break;
  // }
  std::cout << "webserv!" << std::endl;
  return EXIT_SUCCESS;
}

int check_func(int func, std::string error_message) {
  if (func == -1) {
    perror(error_message.c_str());
    exit(EXIT_FAILURE);
  }
  return func;
}
