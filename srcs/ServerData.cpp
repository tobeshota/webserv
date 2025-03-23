#include "ServerData.hpp"

ServerData::ServerData()
    : server_fd(-1), new_socket(0), addrlen(sizeof(address)), port(PORT) {}

ServerData::ServerData(int port)
    : server_fd(-1), new_socket(0), addrlen(sizeof(address)), port(port) {}

ServerData::~ServerData() {}

void ServerData::set_address_data() {
  // アドレスデータの設定
  address.sin_family = AF_INET;  // IPv4アドレスファミリーを指定
  address.sin_addr.s_addr =
      INADDR_ANY;  // 全てのローカルインターフェースにバインド（バインドとは、ソケットをアドレスに関連付けること）
  // 設定されたポート番号を使用
  address.sin_port = htons(port);
}

// ポート番号の取得と設定メソッドを追加
int ServerData::get_port() const { return port; }

void ServerData::set_port(int port) { this->port = port; }

void ServerData::set_server_fd() {
  // ソケットの作成
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    std::exit(EXIT_FAILURE);
  }
  // ソケットの再利用を許可する設定を追加
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    std::exit(EXIT_FAILURE);
  }
}

void ServerData::server_bind() {
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    std::exit(EXIT_FAILURE);
  }
}

void ServerData::server_listen() {
  if (listen(server_fd, MAX_CONNECTION) < 0) {
    perror("listen");
    std::exit(EXIT_FAILURE);
  }
}

void ServerData::server_accept() {
  new_socket =
      accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
  if (new_socket < 0) {
    perror("accept");
    std::exit(EXIT_FAILURE);
  }
}

int ServerData::get_server_fd() const { return server_fd; }

int ServerData::get_new_socket() const { return new_socket; }

const struct sockaddr_in &ServerData::get_address() const { return address; }

int ServerData::get_addrlen() const { return addrlen; }

void ServerData::set_new_socket(int new_socket) {
  this->new_socket = new_socket;
}
