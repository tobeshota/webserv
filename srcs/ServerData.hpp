#pragma once

#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#define PORT 80  //  confからとってくるべき
#define MAX_CONNECTION 128
#define DEFAULT_CONF_PATH "./conf/webserv.conf"

class IServerFunctions {
 public:
  virtual ~IServerFunctions() {}
  virtual void server_bind() = 0;
  virtual void server_listen() = 0;
  virtual void server_accept() = 0;
  virtual void set_address_data() = 0;
  virtual void set_server_fd() = 0;
  virtual int get_server_fd() const = 0;
  virtual int get_new_socket() const = 0;
  const virtual struct sockaddr_in& get_address() const = 0;
  virtual int get_addrlen() const = 0;
  virtual void set_new_socket(int new_socket) = 0;
  virtual int get_port() const = 0;
  virtual void set_port(int port) = 0;
};

class ServerData : public IServerFunctions {
 private:
  int server_fd;
  int new_socket;
  struct sockaddr_in address;
  int addrlen;
  int port;  // ポート番号を保持する変数を追加

 public:
  ServerData();
  ServerData(int port);  // ポート番号を指定するコンストラクタを追加
  ~ServerData();
  void set_address_data();
  void set_server_fd();
  void server_bind();
  void server_listen();
  void server_accept();
  int get_server_fd() const;
  int get_new_socket() const;
  const struct sockaddr_in& get_address() const;
  int get_addrlen() const;
  void set_new_socket(int new_socket);
  int get_port() const;     // ポート番号を取得するメソッド
  void set_port(int port);  // ポート番号を設定するメソッド
};
