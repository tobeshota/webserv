#pragma once

#include <poll.h>
#include <unistd.h>

#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "HTTPResponse.hpp"
#include "PrintResponse.hpp"
#include "ServerData.hpp"

// 前方宣言（循環参照を防ぐため）
class MultiPortServer;

class RunServer {
 private:
  std::vector<pollfd> poll_fds;
  std::map<int, int> client_to_port;  // クライアントFD → サーバーポートのマップ

 public:
  RunServer();
  ~RunServer();
  std::vector<pollfd> &get_poll_fds();

  void run(ServerData &server_data);

  // MultiPortServer対応の関数
  void runMultiPort(MultiPortServer &server);

  void add_poll_fd(pollfd poll_fd);
  void handle_new_connection(int server_fd, int server_port = -1);
  void handle_client_data(size_t client_fd);
  void handle_client_data(size_t client_fd_index, int server_port = -1);

  void process_poll_events(ServerData &server_data);

  // MultiPortServer対応のイベント処理
  void process_poll_events_multiport(MultiPortServer &server);
};
