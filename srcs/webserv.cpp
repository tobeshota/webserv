#include "ParseConf.hpp"
#include "OSInit.hpp"
#include "ParseHTTPRequest.hpp"
#include "GenerateHTTPResponse.hpp"
#include <iostream>

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