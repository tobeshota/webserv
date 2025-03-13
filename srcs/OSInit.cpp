#include "OSInit.hpp"

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
}

void OSInit::close_server_fd(ServerData &server_data) {
  close(server_data.get_server_fd());
}
