#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include "ServerData.hpp"
#include "Poll.hpp"


#define PORT 8080
#define MAX_CONNECTION 3

// サーバーを構築する
// webserv.conf指定のポート番号でのリッスンを受け付ける
class OSInit {
 private:
  /* data */
  ServerData server_data;
  Poll poll_data;
  std::vector<pollfd> poll_fds;

  void process_poll_events();
  void handle_new_connection(int server_fd);
  void handle_client_data(size_t i);
 public:
  OSInit(/* args */);
  ~OSInit();
  // サーバーを構築する
  void initServer();
  int  check_func(int func, std::string error_message);
  void run();

  virtual void set_serverpoll_data();
};
