#include "MultiPortServer.hpp"

MultiPortServer::MultiPortServer() {}

MultiPortServer::~MultiPortServer() {
  // 全サーバーソケットを閉じる
  closeSockets();
}

// 複数ポートをまとめて設定
void MultiPortServer::setPorts(const std::vector<int>& new_ports) {
  ports = new_ports;
}

// サーバーFDのリストを取得
const std::vector<int>& MultiPortServer::getServerFds() const {
  return server_fds;
}

// ポート番号のリストを取得
const std::vector<int>& MultiPortServer::getPorts() const { return ports; }

// FDからポート番号を逆引き
int MultiPortServer::getPortByFd(int fd) const {
  std::map<int, int>::const_iterator it = fd_to_port.find(fd);
  if (it != fd_to_port.end()) {
    return it->second;
  }
  return -1;  // 見つからない場合
}

// 指定されたFDがサーバーFDかどうか
bool MultiPortServer::isServerFd(int fd) const {
  return fd_to_port.find(fd) != fd_to_port.end();
}

// すべてのサーバーソケットをクローズ
void MultiPortServer::closeSockets() {
  for (size_t i = 0; i < server_fds.size(); ++i) {
    if (server_fds[i] >= 0) {
      close(server_fds[i]);
    }
  }
  server_fds.clear();
  fd_to_port.clear();
  addrs.clear();
}

// OSInitで初期化したサーバーFDを追加するメソッド
void MultiPortServer::addServerFd(int fd, int port) {
  if (fd < 0) {
    return;
  }
  server_fds.push_back(fd);
  fd_to_port[fd] = port;

  // アドレス情報も保存（必要に応じて）
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  addrs.push_back(address);

  std::cout << "Added server fd " << fd << " for port " << port << std::endl;
}
