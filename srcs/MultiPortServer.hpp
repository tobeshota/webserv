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

class MultiPortServer {
 private:
  std::vector<int> ports;         // 監視するポート番号のリスト
  std::map<int, int> fd_to_port;  // サーバーFD→ポート番号のマッピング
  std::vector<int> server_fds;            // サーバーFDのリスト
  std::vector<struct sockaddr_in> addrs;  // 各ポートのアドレス情報

 public:
  MultiPortServer();
  ~MultiPortServer();

  // ポートを設定
  void setPorts(const std::vector<int>& ports);

  // 各種ゲッター
  const std::vector<int>& getServerFds() const;
  const std::vector<int>& getPorts() const;
  int getPortByFd(int fd) const;
  bool isServerFd(int fd) const;

  // サーバーソケットをクローズ
  void closeSockets();

  // OSInitと連携するための新しいメソッド
  void addServerFd(int fd, int port);
};
