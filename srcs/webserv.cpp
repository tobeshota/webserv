#include <cstdlib>
#include <iostream>
#include <vector>

#include "OSInit.hpp"
#include "TOMLParser.hpp"

int webserv(int argc, char** argv) {
  (void)argc;
  (void)argv;

  OSInit os;
  RunServer run_server;

  TOMLParser toml_parser;
  Directive* directive = toml_parser.parseFromFile("./conf/webserv.conf");

  printDirective(*directive);  //  for debug

  // C++98スタイルの初期化に修正
  std::vector<int> ports;
  ports.push_back(8080);
  ports.push_back(8081);
  ports.push_back(8082);

  // 各ポート用のServerDataオブジェクトを作成
  std::vector<ServerData*> servers;
  for (size_t i = 0; i < ports.size(); ++i) {
    ServerData* server = new ServerData(ports[i]);
    servers.push_back(server);
  }

  // すべてのサーバーを初期化
  os.initServers(servers);
  // pollのために全サーバーを設定
  os.set_serverspoll_data(servers, run_server);
  // メインループを実行（複数サーバー対応版）
  run_server.run(servers);
  // RAIIにより、サーバーのファイルディスクリプタがクローズされる
  os.close_servers_fds(servers);
  for (size_t i = 0; i < servers.size(); ++i) {
    delete servers[i];
  }

  return EXIT_SUCCESS;
}

// #include <cstdlib>
// #include <iostream>

// #include "OSInit.hpp"
// #include "TOMLParser.hpp"

// int webserv(int argc, char** argv) {
//   (void)argc;
//   (void)argv;

//   ServerData server_data;
//   OSInit os;
//   RunServer run_server;

//   TOMLParser toml_parser;
//   Directive* directive = toml_parser.parseFromFile("./conf/webserv.conf");

//   printDirective(*directive);  //  for debug

//   os.initServer(server_data);
//   os.set_serverpoll_data(server_data, run_server);
//   // メインループを実行
//   run_server.run(server_data);
//   os.close_server_fd(server_data);
//   return EXIT_SUCCESS;
// }
