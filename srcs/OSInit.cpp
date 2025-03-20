#include "OSInit.hpp"

OSInit::OSInit() {}

OSInit::~OSInit() {}

// 単一サーバーを構築する
void OSInit::initServer(ServerData &server_data) {
  // サーバーの構築
  server_data.set_address_data();
  server_data.set_server_fd();
  server_data.server_bind();
  server_data.server_listen();

  std::cout << "Server initialized on port " << server_data.get_port()
            << std::endl;
}

// 複数サーバーを構築する
void OSInit::initServers(std::vector<ServerData *> &servers) {
  for (size_t i = 0; i < servers.size(); ++i) {
    // 各サーバーの構築
    servers[i]->set_address_data();
    servers[i]->set_server_fd();
    servers[i]->server_bind();
    servers[i]->server_listen();

    std::cout << "Server #" << i << " initialized on port "
              << servers[i]->get_port() << std::endl;
  }

  std::cout << "Startup complete! All servers are running." << std::endl;
}

// サーバーのファイルディスクリプタを poll システムコールで監視するための設定
void OSInit::set_serverpoll_data(ServerData &server_data,
                                 RunServer &run_server) {
  // poll_fdsの初期設定
  pollfd server_fd_poll;
  // サーバーのファイルディスクリプタを監視するための設定
  server_fd_poll.fd = server_data.get_server_fd();
  // ファイルディスクリプタが読み込み可能（新しい接続がある）かどうかを監視するためのイベント
  server_fd_poll.events = POLLIN;
  // 設定した pollfd 構造体を run_server に追加
  run_server.add_poll_fd(server_fd_poll);
  // サーバーFDとServerDataオブジェクトのマッピングを追加
  run_server.add_server_mapping(server_data.get_server_fd(), &server_data);
}

// 複数サーバーのファイルディスクリプタを監視設定
void OSInit::set_serverspoll_data(std::vector<ServerData *> &servers,
                                  RunServer &run_server) {
  // 各サーバーごとに監視設定を追加
  for (size_t i = 0; i < servers.size(); ++i) {
    pollfd server_fd_poll;
    server_fd_poll.fd = servers[i]->get_server_fd();
    server_fd_poll.events = POLLIN;
    run_server.add_poll_fd(server_fd_poll);
    run_server.add_server_mapping(servers[i]->get_server_fd(), servers[i]);
  }
}

void OSInit::close_server_fd(ServerData &server_data) {
  close(server_data.get_server_fd());
}

void OSInit::close_servers_fds(std::vector<ServerData *> &servers) {
  for (size_t i = 0; i < servers.size(); ++i) {
    close(servers[i]->get_server_fd());
  }
}
