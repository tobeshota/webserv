#include "RunServer.hpp"

#include "DeleteClientMethod.hpp"
#include "GET.hpp"
#include "HTTPRequestParser.hpp"
#include "MultiPortServer.hpp"
#include "POST.hpp"
#include "TOMLParser.hpp"

RunServer::RunServer() { setConfPath(DEFAULT_CONF_PATH); }

RunServer::~RunServer() {}

std::string RunServer::getConfPath() { return _confPath; }
void RunServer::setConfPath(std::string confPath) { _confPath = confPath; }

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
  get_poll_fds().push_back(client_fd_poll);
}

Handler *getHTTPMethodHandler(const std::string &HTTPMethod,
                              Directive rootDirective,
                              HTTPRequest httpRequest) {
  // switch分岐でHTTPメソッドに対応するハンドラを返す
  if (HTTPMethod == "GET") {
    return new GET(rootDirective, httpRequest);
  } else if (HTTPMethod == "POST") {
    return new POST(rootDirective, httpRequest);
  } else if (HTTPMethod == "DELETE") {
    return new DeleteClientMethod(httpRequest, rootDirective);
  } else {
    // 未対応のHTTPメソッドの場合はGETハンドラを返す
    // GETハンドラは405 Method Not Allowedを返す
    return new GET(rootDirective, httpRequest);
  }
}

// std::vector<std::string>の要素に"指定のメソッド"が含まれているか
static bool isContain(std::vector<std::string> vec, std::string str) {
  for (std::vector<std::string>::iterator itr = vec.begin(); itr != vec.end();
       ++itr) {
    if (*itr == str) {
      return true;
    }
  }
  return false;
}

// クライアントからのデータを処理する関数
void RunServer::handle_client_data(size_t client_fd, std::string receivedPort) {
  char buffer[4096] = {0};  // バッファを初期化
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
  Directive *rootDirective = NULL;
  buffer[bytes_read] = '\0';
  try {
    // HTTPリクエストをパース
    HTTPRequestParser parser;
    if (parser.feed(buffer, strlen(buffer))) {
      if (parser.hasError()) {
        throw std::invalid_argument("Failed to parse HTTP request");
      }
    } else {
      throw std::invalid_argument(
          "Failed to parse HTTP request: more data needed");
    }
    HTTPRequest httpRequest = parser.createRequest();
    // ConfigからDirectiveを取得
    TOMLParser toml_parser;
    rootDirective = toml_parser.parseFromFile(getConfPath());
    if (rootDirective == NULL)
      throw std::invalid_argument("Failed to parse Conf");

    // HTTPレスポンスオブジェクトを作成
    HTTPResponse httpResponse;

    // 鎖をつなげる
    PrintResponse printResponse(get_poll_fds()[client_fd].fd);
    GenerateHTTPResponse generateHTTPResponse(*rootDirective, httpRequest);
    generateHTTPResponse.setNextHandler(&printResponse);

    // 指定ホストは指定のポートをリッスンするか
    std::string host = httpRequest.getServerName();
    const Directive *hostDirective = rootDirective->findDirective(host);
    std::vector<std::string> hostReveivablePorts =
        hostDirective->getValues("listen");
    if (isContain(hostReveivablePorts, receivedPort)) {
      Handler *handler = getHTTPMethodHandler(httpRequest.getMethod(),
                                              *rootDirective, httpRequest);
      handler->setNextHandler(&generateHTTPResponse);
      handler->handleRequest(httpResponse);
      delete handler;
    } else {
      httpResponse.setHttpStatusCode(400);  // Bad Request
      generateHTTPResponse.handleRequest(httpResponse);
    }
  } catch (const std::exception &e) {
    std::cerr << "Error handling client data: " << e.what() << std::endl;
  }
  delete rootDirective;
  // Connection: closeの場合は接続を閉じる
  close(get_poll_fds()[client_fd].fd);
  get_poll_fds().erase(get_poll_fds().begin() + client_fd);
}

static std::string int2str(int nb) {
  std::stringstream ss;
  ss << nb;
  return ss.str();
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
        std::cout << "New connection on port " << port << std::endl;
        handle_new_connection(current_fd);
      } else {
        // クライアント接続からのデータ
        int server_port = server.getPortByFd(current_fd);
        handle_client_data(i, int2str(server_port));
      }
    }
  }
}
