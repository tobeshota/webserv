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

  void initServer(ServerData &server_data);

  void close_server_fd(ServerData &server_data);
};
