#pragma once

#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "RunServer.hpp"
#include "ServerData.hpp"

// サーバーを構築する
// webserv.conf指定のポート番号でのリッスンを受け付ける
class OSInit {
 public:
  OSInit();
  ~OSInit();

  // 単一サーバーを構築する
  void initServer(ServerData &server_data);

  // 複数サーバーを構築する
  void initServers(std::vector<ServerData *> &servers);

  // 単一サーバーの監視設定
  virtual void set_serverpoll_data(ServerData &server_data,
                                   RunServer &run_server);

  // 複数サーバーの監視設定
  virtual void set_serverspoll_data(std::vector<ServerData *> &servers,
                                    RunServer &run_server);

  // 単一サーバーのファイルディスクリプタをクローズ
  void close_server_fd(ServerData &server_data);

  // 複数サーバーのファイルディスクリプタをクローズ
  void close_servers_fds(std::vector<ServerData *> &servers);
};
