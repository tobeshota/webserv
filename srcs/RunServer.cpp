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
void RunServer::handle_new_connection(int server_fd, int server_port) {
  int new_socket = accept(server_fd, NULL, NULL);
  if (new_socket == -1) {
    perror("accept");
    return;
  }

  // クライアントFDとサーバーポートの対応を保存
  client_to_port[new_socket] = server_port;
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
  const size_t BUFFER = 4096;
  std::string complete_request;
  char buffer[BUFFER] = {0};
  ssize_t bytes_read;
  int client_socket = get_poll_fds()[client_fd].fd;
  bool received_full_request = false;

  // データを受信し、完全なリクエストを構築
  while (!received_full_request) {
    bytes_read = recv(client_socket, buffer, BUFFER - 1, 0);

    if (bytes_read <= 0) {
      if (bytes_read == -1) {
        perror("recv");
      }
      close(client_socket);
      get_poll_fds().erase(get_poll_fds().begin() + client_fd);
      client_to_port.erase(client_socket);
      return;
    }

    buffer[bytes_read] = '\0';
    // 受信したデータを完全なリクエストに追加
    complete_request.append(buffer, bytes_read);

    // Content-Lengthがある場合、完全なリクエストを受信したか確認
    // ヘッダーとボディの区切り（空行）を見つけ、Content-Lengthに基づいてボディの長さを確認
    size_t header_end = complete_request.find("\r\n\r\n");
    // header_end != std::string::npos なら、ヘッダー部分の受信は完了している。
    if (header_end != std::string::npos) {
      std::string header_part = complete_request.substr(0, header_end);
      std::string lower_header;
      lower_header.reserve(header_part.length());
      for (size_t i = 0; i < header_part.length(); ++i) {
        lower_header += std::tolower(header_part[i]);
      }

      // Content-Lengthを探す
      size_t content_length_pos = lower_header.find("content-length:");
      if (content_length_pos != std::string::npos) {
        // Content-Lengthの値を取得
        size_t value_start = lower_header.find_first_not_of(
            " \t", content_length_pos + 15);  // 15は"content-length:"の長さ
        if (value_start != std::string::npos) {
          size_t value_end = lower_header.find_first_of("\r\n", value_start);
          if (value_end != std::string::npos) {
            std::string length_str =
                lower_header.substr(value_start, value_end - value_start);
            std::istringstream iss(length_str);
            size_t content_length = 0;
            iss >> content_length;

            // ボディ部分の現在の長さを計算
            size_t body_length = complete_request.length() -
                                 (header_end + 4);  // 4は\r\n\r\nの長さ

            // ボディが完全に受信されたか確認
            if (body_length >= content_length) {
              received_full_request = true;
            }
          }
        }
      } else {
        // Content-Lengthがない場合は、ヘッダーを受信した時点で完了と見なす
        received_full_request = true;
      }
    }
  }

  Directive *rootDirective = NULL;
  try {
    // HTTPリクエストをパース
    HTTPRequestParser parser;
    if (parser.feed(complete_request.c_str(), complete_request.size())) {
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

    // URLリダイレクトが指定されている場合，httpRequestのURLをリダイレクト先に変更する
    // URLリダイレクトとはすなわち，HTTPリクエストのURLを変更することであるため．
    GenerateHTTPResponse search(*rootDirective, httpRequest);
    if (search.getDirectiveValue("return") != "") {
      httpRequest.setURL(search.getDirectiveValue("return"));
    }

    // 鎖をつなげる
    PrintResponse printResponse(get_poll_fds()[client_fd].fd);
    GenerateHTTPResponse generateHTTPResponse(*rootDirective, httpRequest);
    generateHTTPResponse.setNextHandler(&printResponse);

    const Directive *hostDirective =
        rootDirective->findDirective(httpRequest.getServerName());
    if (hostDirective == NULL ||
        !isContain(hostDirective->getValues("listen"), receivedPort)) {
      // 指定ホストは指定のポートをリッスンしない場合
      httpResponse.setHttpStatusCode(400);  // Bad Request
      generateHTTPResponse.handleRequest(httpResponse);
    } else if (isContain(search.getDirectiveValues("deny"),
                         httpRequest.getMethod())) {
      // 実行メソッドが許可されていない場合
      httpResponse.setHttpStatusCode(405);  // Method Not Allowed
      generateHTTPResponse.handleRequest(httpResponse);
    } else {
      // 通常
      Handler *handler = getHTTPMethodHandler(httpRequest.getMethod(),
                                              *rootDirective, httpRequest);
      handler->setNextHandler(&generateHTTPResponse);
      handler->handleRequest(httpResponse);
      delete handler;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error handling client data: " << e.what() << std::endl;
  }
  delete rootDirective;
  // Connection: closeの場合は接続を閉じる
  close(get_poll_fds()[client_fd].fd);
  get_poll_fds().erase(get_poll_fds().begin() + client_fd);
  client_to_port.erase(get_poll_fds()[client_fd].fd);
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
        handle_new_connection(current_fd, port);
      } else {
        // クライアント接続からのデータ
        int server_port = client_to_port[current_fd];
        handle_client_data(i, int2str(server_port));
      }
    }
  }
}
