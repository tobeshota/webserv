#pragma once

#include <unistd.h>

#include <iostream>
#include <string>

#include "RunServer.hpp"
#include "ServerData.hpp"
#include "webserv.hpp"

#define PORT 8080
#define MAX_CONNECTION 3

// サーバーを構築する
// webserv.conf指定のポート番号でのリッスンを受け付ける
class OSInit {
 public:
  OSInit(/* args */);
  ~OSInit();
  // サーバーを構築する
  void initServer(ServerData &server_data);
  virtual void set_serverpoll_data(ServerData &server_data,
                                   RunServer &run_server);
  void close_server_fd(ServerData &server_data);
};
