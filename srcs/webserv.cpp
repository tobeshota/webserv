#include <cstdlib>
#include <iostream>

#include "OSInit.hpp"
#include "TOMLParser.hpp"

int webserv(int argc, char** argv) {
  ServerData server_data;
  OSInit os;
  RunServer run_server;

  if (argc == 2) {
    run_server.setConfPath(argv[1]);
  }
  os.initServer(server_data);
  os.set_serverpoll_data(server_data, run_server);
  // メインループを実行
  run_server.run(server_data);
  os.close_server_fd(server_data);
  return EXIT_SUCCESS;
}
