#pragma once

#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define MAX_CONNECTION 3

class MultiPortServer {
 private:
  std::vector<int> ports;                // 監視するポート番号のリスト
  std::map<int, int> fd_to_port;         // サーバーFD→ポート番号のマッピング
  std::vector<int> server_fds;           // サーバーFDのリスト
  std::vector<struct sockaddr_in> addrs; // 各ポートのアドレス情報

 public:
  MultiPortServer();
  ~MultiPortServer();

  // ポートを設定
  void addPort(int port);
  void setPorts(const std::vector<int>& ports);

  // サーバーソケットを初期化
  bool initializeSockets();

  // 各種ゲッター
  const std::vector<int>& getServerFds() const;
  const std::vector<int>& getPorts() const;
  int getPortByFd(int fd) const;
  bool isServerFd(int fd) const;
  
  // サーバーソケットをクローズ
  void closeSockets();
};
