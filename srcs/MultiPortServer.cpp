
#include "MultiPortServer.hpp"

MultiPortServer::MultiPortServer() {}

MultiPortServer::~MultiPortServer() {
  // 全サーバーソケットを閉じる
  closeSockets();
}

// 単一のポートを追加
void MultiPortServer::addPort(int port) {
  ports.push_back(port);
}

// 複数ポートをまとめて設定
void MultiPortServer::setPorts(const std::vector<int>& new_ports) {
  ports = new_ports;
}

// サーバーソケットを初期化
bool MultiPortServer::initializeSockets() {
  // 既存のソケットをクローズ
  closeSockets();
  
  // 配列をクリア
  server_fds.clear();
  fd_to_port.clear();
  addrs.clear();

  bool success = false;

  // 各ポートに対してソケットを作成
  for (size_t i = 0; i < ports.size(); ++i) {
    int port = ports[i];
    int server_fd;
    
    // ソケット作成
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      std::cerr << "ソケット作成失敗: ポート " << port << std::endl;
      continue;
    }
    
    // ソケットの再利用設定
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      std::cerr << "setsockopt失敗: ポート " << port << std::endl;
      close(server_fd);
      continue;
    }
    
    // アドレス設定
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // バインド
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      std::cerr << "バインド失敗: ポート " << port << std::endl;
      close(server_fd);
      continue;
    }
    
    // リッスン開始
    if (listen(server_fd, MAX_CONNECTION) < 0) {
      std::cerr << "Listen失敗: ポート " << port << std::endl;
      close(server_fd);
      continue;
    }
    
    // 成功したらリストに追加
    server_fds.push_back(server_fd);
    fd_to_port[server_fd] = port;
    addrs.push_back(address);
    success = true;
    
    std::cout << "ポート " << port << " のサーバー初期化完了 (fd: " << server_fd << ")" << std::endl;
  }
  
  return success; // 少なくとも1つのポートが成功したかどうか
}

// サーバーFDのリストを取得
const std::vector<int>& MultiPortServer::getServerFds() const {
  return server_fds;
}

// ポート番号のリストを取得
const std::vector<int>& MultiPortServer::getPorts() const {
  return ports;
}

// FDからポート番号を逆引き
int MultiPortServer::getPortByFd(int fd) const {
  std::map<int, int>::const_iterator it = fd_to_port.find(fd);
  if (it != fd_to_port.end()) {
    return it->second;
  }
  return -1; // 見つからない場合
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
