// #include "RunServer.hpp"
// #include "OSInit.hpp"
#include "webserv.hpp"

RunServer::RunServer() {}

RunServer::~RunServer() {}

std::vector<pollfd> RunServer::get_poll_fds() { return poll_fds; }

// メインループを実行する関数
void RunServer::run(ServerData &server_data) {
  while (true) {
    // pollシステムコールを呼び出し、イベントを待つ
    int poll_count = 
        poll(poll_fds.data(), poll_fds.size(), -1);
    // デバッグ用にpoll_countを出力
    std::cout << poll_count << std::endl;
    // pollイベントを処理
    process_poll_events(server_data);
  }
}

// 新しいpollfdを追加する関数
void RunServer::add_poll_fd(pollfd poll_fd) { poll_fds.push_back(poll_fd); }

// 新しい接続を処理する関数
void RunServer::handle_new_connection(int server_fd) {
  int new_socket = accept(server_fd, nullptr, nullptr);
  if (new_socket == -1) {
    perror("accept");
    return;
  }
  std::cout << "New connection accepted" << std::endl;

  pollfd client_fd_poll;
  client_fd_poll.fd = new_socket;
  client_fd_poll.events = POLLIN;
  get_poll_fds().push_back(client_fd_poll);
}

// クライアントからのデータを処理する関数
void RunServer::handle_client_data(size_t i) {
  char buffer[1024];
  ssize_t bytes_read = read(get_poll_fds()[i].fd, buffer, sizeof(buffer));
  if (bytes_read == -1) {
    perror("read");
    close(get_poll_fds()[i].fd);
    get_poll_fds().erase(get_poll_fds().begin() + i);
    --i;
  } else if (bytes_read == 0) {
    std::cout << "Client disconnected" << std::endl;
    close(get_poll_fds()[i].fd);
    get_poll_fds().erase(get_poll_fds().begin() + i);
    --i;
  } else {
    buffer[bytes_read] = '\0';
    std::cout << "Received: " << buffer << std::endl;
    ssize_t bytes_sent = write(get_poll_fds()[i].fd, buffer, bytes_read);
    if (bytes_sent == -1) {
      perror("write");
      close(get_poll_fds()[i].fd);
      get_poll_fds().erase(get_poll_fds().begin() + i);
      --i;
    }
  }
}

// pollイベントを処理する関数。ポーリングだけど、接続数が１００未満程度なら問題ない
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
