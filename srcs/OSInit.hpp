#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include "ServerData.hpp"


#define PORT 8080
#define MAX_CONNECTION 3

// サーバーを構築する
// webserv.conf指定のポート番号でのリッスンを受け付ける
class OSInit {
 private:
  /* data */
  ServerData server_data;
 public:
  OSInit(/* args */);
  ~OSInit();
  // サーバーを構築する
  void initServer();
};
