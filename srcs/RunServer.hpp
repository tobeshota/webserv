#pragma once

#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "PrintResponse.hpp"  //一時的に追加。本来正常レスポンスクラスで呼び出し
#include "ServerData.hpp"

class RunServer {
 private:
  std::vector<pollfd> poll_fds;

 public:
  RunServer(/* args */);
  ~RunServer();

  void run(ServerData &server_data);
  void add_poll_fd(pollfd poll_fd);
  std::vector<pollfd> &get_poll_fds();
  void close_server_fd();

  void process_poll_events(ServerData &server_data);
  void handle_new_connection(int server_fd);
  void handle_client_data(size_t i, RunServer &run_server);
};
