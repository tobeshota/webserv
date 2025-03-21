#include "OSInit.hpp"

OSInit::OSInit() {}

OSInit::~OSInit() {}

void OSInit::initServer(ServerData &server_data) {
  try {
    // サーバーの構築
    server_data.set_address_data();
    server_data.set_server_fd();
    server_data.server_bind();
    server_data.server_listen();
    
    std::cout << "Server initialized on port " << server_data.get_port()
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error initializing server on port " << server_data.get_port() 
              << ": " << e.what() << std::endl;
    
    // エラー発生時にソケットをクローズ
    close_server_fd(server_data);
  }
}

void OSInit::close_server_fd(ServerData &server_data) {
  if (server_data.get_server_fd() >= 0) {
    close(server_data.get_server_fd());
  }
}
