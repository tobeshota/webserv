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

class RunServer {
 private:
  std::vector<pollfd> poll_fds;
  std::map<int, ServerData*> server_mappings; // サーバーFDとServerDataオブジェクトのマッピング

 public:
  RunServer();
  ~RunServer();
  std::vector<pollfd> &get_poll_fds();
  
  // サーバーFDとServerDataオブジェクトのマッピングを追加
  void add_server_mapping(int server_fd, ServerData* server);
  
  // 単一サーバー対応の関数
  void run(ServerData &server_data);
  
  // 複数サーバー対応の関数
  void run(std::vector<ServerData*> &servers);
  
  void add_poll_fd(pollfd poll_fd);
  void handle_new_connection(int server_fd);
  void handle_client_data(size_t client_fd);
  
  // 単一サーバー対応のイベント処理
  void process_poll_events(ServerData &server_data);
  
  // 複数サーバー対応のイベント処理
  void process_poll_events(std::vector<ServerData*> &servers);
};
