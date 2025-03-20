#include "RunServer.hpp"

#include "MultiPortServer.hpp"

RunServer::RunServer() {}

RunServer::~RunServer() {}

std::vector<pollfd> &RunServer::get_poll_fds() { return poll_fds; }

// メインループを実行する関数
void RunServer::run(ServerData &server_data) {
  while (true) {
    // pollシステムコールを呼び出し、イベントを待つ
    int poll_count = poll(poll_fds.data(), poll_fds.size(), -1);
    // デバッグ用にpoll_countを出力
    std::cout << poll_count << std::endl;
    // pollイベントを処理
    process_poll_events(server_data);
  }
}

// MultiPortServer用のイベントループ実装
void RunServer::runMultiPort(MultiPortServer &server) {
  while (true) {
    // pollシステムコールを呼び出し、イベントを待つ
    int poll_count = poll(poll_fds.data(), poll_fds.size(), -1);

    // デバッグ用出力
    std::cout << "Poll イベント数: " << poll_count << std::endl;

    // イベント処理
    process_poll_events_multiport(server);
  }
}

// 新しいpollfdを追加する関数
void RunServer::add_poll_fd(pollfd poll_fd) { poll_fds.push_back(poll_fd); }

// 新しい接続を処理する関数
void RunServer::handle_new_connection(int server_fd) {
  int new_socket = accept(server_fd, NULL, NULL);
  if (new_socket == -1) {
    perror("accept");
    return;
  }
  std::cout << "New connection accepted" << std::endl;

  pollfd client_fd_poll;
  client_fd_poll.fd = new_socket;
  client_fd_poll.events = POLLIN;
  std::cout << "poll_fds.size() " << get_poll_fds().size() << std::endl;
  get_poll_fds().push_back(client_fd_poll);
  std::cout << "poll_fds.size() " << get_poll_fds().size() << std::endl;
}

// クライアントからのデータを処理する関数
void RunServer::handle_client_data(size_t client_fd) {
  char buffer[4096];
  ssize_t bytes_read =
      recv(get_poll_fds()[client_fd].fd, buffer, sizeof(buffer) - 1, 0);

  // クライアント切断またはエラーの処理
  if (bytes_read <= 0) {
    if (bytes_read == -1) {
      perror("recv");
    }
    close(get_poll_fds()[client_fd].fd);
    get_poll_fds().erase(get_poll_fds().begin() + client_fd);
    return;
  }

  buffer[bytes_read] = '\0';
  std::cout << "Handling client data" << std::endl;
  std::cout << "Received: " << buffer << std::endl;

  try {
    PrintResponse print_response(get_poll_fds()[client_fd].fd);
    HTTPResponse response;
    // HTTPRequest request(buffer);

    // エコーバック（テスト用）
    send(get_poll_fds()[client_fd].fd, buffer, bytes_read, MSG_NOSIGNAL);

    // 実際のレスポンス処理
    print_response.handleRequest(response);
  } catch (const std::exception &e) {
    std::cerr << "Error handling client data: " << e.what() << std::endl;
    close(get_poll_fds()[client_fd].fd);
    get_poll_fds().erase(get_poll_fds().begin() + client_fd);
  }
}

// pollにより、イベント発生してからforをするので、busy-waitではない
//  pollイベントを処理する関数。ポーリングだけど、「イベントが来るまで待機する」ので
//  busy-wait ではない
void RunServer::process_poll_events(ServerData &server_data) {
  // 監視対象のファイルディスクリプタ（pollfdリスト）をループでチェック
  for (size_t i = 0; i < get_poll_fds().size(); ++i) {
    // pollfd構造体のreventsにPOLLIN（読み込み可能イベント）がセットされている場合
    if (get_poll_fds()[i].revents & POLLIN) {
      // サーバーのファイルディスクリプタがイベントを発生させた場合
      if (get_poll_fds()[i].fd == server_data.get_server_fd()) {
        // 新しい接続を受け入れる処理を実行
        handle_new_connection(server_data.get_server_fd());
      } else {
        // クライアントから送信されたデータを処理
        // 該当するクライアントのデータを処理
        handle_client_data(i);
      }
    }
  }
}

// MultiPortServer用のイベント処理
void RunServer::process_poll_events_multiport(MultiPortServer &server) {
  // すべてのファイルディスクリプタをチェック
  for (size_t i = 0; i < get_poll_fds().size(); ++i) {
    // イベントが発生したかチェック
    if (get_poll_fds()[i].revents & POLLIN) {
      int current_fd = get_poll_fds()[i].fd;

      // サーバーソケットのイベントかチェック
      if (server.isServerFd(current_fd)) {
        int port = server.getPortByFd(current_fd);
        std::cout << "ポート " << port << " に新しい接続要求" << std::endl;
        handle_new_connection(current_fd);
      } else {
        // クライアント接続からのデータ
        handle_client_data(i);
      }
    }
  }
}
