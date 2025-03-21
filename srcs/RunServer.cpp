#include "RunServer.hpp"

#include "MultiPortServer.hpp"

RunServer::RunServer() {}

RunServer::~RunServer() {}

std::vector<pollfd> &RunServer::get_poll_fds() { return poll_fds; }

// MultiPortServer用のイベントループ実装
void RunServer::runMultiPort(MultiPortServer &server) {
  while (true) {
    // pollシステムコールを呼び出し、イベントを待つ
    poll(poll_fds.data(), poll_fds.size(), -1);
    // イベント処理
    process_poll_events_multiport(server);
  }
}

// 新しいpollfdを追加する関数
void RunServer::add_poll_fd(pollfd poll_fd) { poll_fds.push_back(poll_fd); }

// 新しい接続を処理する関数
void RunServer::handle_new_connection(int server_fd, int server_port) {
  int new_socket = accept(server_fd, NULL, NULL);
  if (new_socket == -1) {
    perror("accept");
    return;
  }

  // // サーバーポートが指定されていなければgetsocknameで取得
  // if (server_port == -1) {
  //   struct sockaddr_in addr;
  //   socklen_t addr_len = sizeof(addr);
  //   if (getsockname(server_fd, (struct sockaddr*)&addr, &addr_len) == 0) {
  //     server_port = ntohs(addr.sin_port);
  //   }
  // }

  std::cout << "New connection accepted on port " << server_port << std::endl;

  // クライアントFDとサーバーポートの対応を保存
  client_to_port[new_socket] = server_port;

  pollfd client_fd_poll;
  client_fd_poll.fd = new_socket;
  client_fd_poll.events = POLLIN;
  get_poll_fds().push_back(client_fd_poll);
}

// クライアントからのデータを処理する関数
void RunServer::handle_client_data(size_t client_fd_index, int server_port) {
  char buffer[4096];
  int client_fd = get_poll_fds()[client_fd_index].fd;

  // // server_port が指定されていない場合は getsockname で取得
  // if (server_port == -1) {
  //   struct sockaddr_in addr;
  //   socklen_t addr_len = sizeof(addr);
  //   if (getsockname(client_fd, (struct sockaddr*)&addr, &addr_len) == 0) {
  //     server_port = ntohs(addr.sin_port);
  //   }
  // }

  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

  // クライアント切断またはエラーの処理
  if (bytes_read <= 0) {
    if (bytes_read == -1) {
      perror("recv");
    }
    close(client_fd);
    get_poll_fds().erase(get_poll_fds().begin() + client_fd_index);
    return;
  }

  buffer[bytes_read] = '\0';
  std::cout << "Handling client data on port " << server_port << std::endl;
  std::cout << "Received: " << buffer << std::endl;

  try {
    PrintResponse print_response(client_fd);
    HTTPResponse response;
    // HTTPRequest request(buffer);

    // エコーバック（テスト用）
    send(client_fd, buffer, bytes_read, MSG_NOSIGNAL);

  } catch (const std::exception &e) {
    std::cerr << "Error handling client data on port " << server_port << ": "
              << e.what() << std::endl;
    close(client_fd);
    get_poll_fds().erase(get_poll_fds().begin() + client_fd_index);
  }
}

// MultiPortServer用のイベント処理
void RunServer::process_poll_events_multiport(MultiPortServer &server) {
  // pollfd構造体のreventsにPOLLIN（読み込み可能イベント）がセットされている場合
  for (size_t i = 0; i < get_poll_fds().size(); ++i) {
    // サーバーのファイルディスクリプタがイベントを発生させた場合
    if (get_poll_fds()[i].revents & POLLIN) {
      int current_fd = get_poll_fds()[i].fd;
      // 新しい接続を受け入れる処理を実行
      if (server.isServerFd(current_fd)) {
        int port = server.getPortByFd(current_fd);
        handle_new_connection(current_fd, port);
      } else {
        // マップから保存したポート情報を取得
        int server_port = client_to_port[current_fd];
        handle_client_data(i, server_port);
      }
    }
  }
}
