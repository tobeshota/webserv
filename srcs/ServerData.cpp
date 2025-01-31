#include "ServerData.hpp"
#include "OSInit.hpp"

ServerData::ServerData()
    : server_fd(0), new_socket(0), addrlen(sizeof(address)) {}

ServerData::~ServerData() {}

void ServerData::set_address_data() {
  // アドレスデータの設定
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  // ここにwebserv.confで指定されたポート番号を入れる。PORTは仮
  address.sin_port = htons(PORT);
}

void ServerData::set_server_fd() {
  // ソケットの作成
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
}

void ServerData::server_bind() {
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
}

void ServerData::server_listen() {
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

void ServerData::server_accept() {
  new_socket =
      accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
  if (new_socket < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }
}

int ServerData::get_server_fd() const { return server_fd; }

int ServerData::get_new_socket() const { return new_socket; }

struct sockaddr_in ServerData::get_address() const { return address; }

int ServerData::get_addrlen() const { return addrlen; }

void ServerData::set_new_socket(int new_socket) {
  this->new_socket = new_socket;
}
