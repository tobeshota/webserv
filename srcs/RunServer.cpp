#include "RunServer.hpp"

// #include "HTTPHandleSuccess.hpp"

RunServer::RunServer() {}

RunServer::~RunServer() {}

std::vector<pollfd> &RunServer::get_poll_fds() { return poll_fds; }

// メインループを実行する関数
void RunServer::run(ServerData &server_data) {
  while (true) {
    // pollシステムコールを呼び出し、イベントを待つ
    int poll_count = poll(poll_fds.data(), poll_fds.size(), -1);
    // デバッグ用にpoll_countを出力
    std::cout << "poll_count" << poll_count << std::endl;
    // pollイベントを処理
    process_poll_events(server_data);
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
  std::cout << "before--handle_new_connection:poll_fds.size() "
            << get_poll_fds().size() << std::endl;
  get_poll_fds().push_back(client_fd_poll);
  std::cout << "after--handle_new_connection:poll_fds.size() "
            << get_poll_fds().size() << std::endl;
}

// クライアントからのデータを処理する関数
void RunServer::handle_client_data(size_t client_fd) {
  PrintResponse print_response(get_poll_fds()[client_fd].fd);
  // HTTPHandleSuccess success_response;
  HTTPResponse response;

  char buffer[4096];
  ssize_t bytes_read =
      recv(get_poll_fds()[client_fd].fd, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read <= 0) {
    if (bytes_read == -1) {
      perror("read");
    }
    close(get_poll_fds()[client_fd].fd);
    get_poll_fds().erase(get_poll_fds().begin() + client_fd);
    return;
  }

  buffer[bytes_read] = '\0';


  // メソッドを実行
  //  exec_method(http request parser)
  //  create_response_data(run_server, i)
  // 正常のレスポンスを返す
  //  http handle successから、レスポンスを取得
  //  HTTPHandleSuccess::ceateResponseData()
  //  printclassで、レスポンスを送信
   print_response.handleRequest(response);
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
