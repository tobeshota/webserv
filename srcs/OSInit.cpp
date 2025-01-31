#include "OSInit.hpp"

#include "webserv.hpp"

OSInit::OSInit() {}

OSInit::~OSInit() {}

// サーバーを構築する
void OSInit::initServer(ServerData &server_data) {
  // サーバーの構築
  server_data.set_address_data();
  server_data.set_server_fd();
  server_data.server_bind();
  server_data.server_listen();

  std::cout << "Startup complete!, Start-up completed!" << std::endl;
  std::cout
      << " The number of people who can connect to this server remaining is "
      << MAX_CONNECTION << std::endl;
}

void OSInit::set_serverpoll_data(ServerData &server_data,
                                 RunServer &run_server) {
  // poll_fdsの初期設定
  pollfd server_fd_poll;
  server_fd_poll.fd = server_data.get_server_fd();
  server_fd_poll.events = POLLIN;
  run_server.add_poll_fd(server_fd_poll);
}

void OSInit::close_server_fd(ServerData &server_data) {
  close(server_data.get_server_fd());
}
