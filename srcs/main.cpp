// #include "OSInit.hpp"
// #include "RunServer.hpp"
// #include "ServerData.hpp"
#include "TOMLParser.hpp"
#include "webserv.hpp"

// int main(int argc, char **argv) { return webserv(argc, argv); }

// this is OSInit's main
int main() {
  ServerData server_data;
  OSInit os;
  RunServer run_server;

  TOMLParser toml_parser;
  Directive* directive = toml_parser.parseFromFile("./conf/webserv.conf");
  printDirective(*directive);   //  for debug

  os.initServer(server_data);
  os.set_serverpoll_data(server_data, run_server);
  // メインループを実行
  run_server.run(server_data);
  os.close_server_fd(server_data);
  // webserv.conf指定のポート番号でのリッスンを受け付ける
  std::cout << "initServer" << std::endl;
  return 0;
}
