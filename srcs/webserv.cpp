#include <cstdlib>
#include <iostream>
#include <vector>

#include "MultiPortServer.hpp"
#include "OSInit.hpp"
#include "RunServer.hpp"  // 明示的にインクルード
#include "TOMLParser.hpp"

// stoiの再実装．string型の文字列を数値として読み取り，int型の値に変換する
static int string_to_int(const std::string str) {
  int result;

  std::istringstream iss(str);
  if (!(iss >> result)) return (-1);
  return (result);
}

//// 複数のポートを用意（本来はconfファイルから取得）の関数を作る
std::vector<int> getPorts() {
  std::vector<int> ports;
  TOMLParser toml_parser;
  Directive* directive = toml_parser.parseFromFile("./conf/webserv.conf");
  if (directive == NULL) {
    std::cerr << "Failed to parse configuration file" << std::endl;
    return ports;
  }
  // ポート番号を取得
  std::vector<std::string> listen_directives = directive->getValues("listen");
  for (size_t i = 0; i < listen_directives.size(); ++i) {
    std::string port_str = listen_directives[i];
    int port = string_to_int(port_str);
    ports.push_back(port);
  }
  delete directive;
  return ports;
}

int webserv(int argc, char** argv) {
  ServerData server_data;
  OSInit osInit;
  RunServer run_server;
  std::vector<int> ports = getPorts();

  if (argc == 2) {
    run_server.setConfPath(argv[1]);
  }

  // マルチポートサーバーを作成
  MultiPortServer server;
  server.setPorts(ports);

  bool success = false;

  // 各ポートに対してServerDataを作成し、OSInitを使用して初期化
  for (size_t i = 0; i < ports.size(); ++i) {
    ServerData server_data(ports[i]);
    osInit.initServer(server_data);

    // 初期化したサーバーFDを追加
    int server_fd = server_data.get_server_fd();
    if (server_fd >= 0) {
      server.addServerFd(server_fd, ports[i]);
      success = true;
    }
  }

  if (!success) {
    std::cerr << "Failed to initialize sockets" << std::endl;
    return EXIT_FAILURE;
  }

  // pollのために各サーバーFDを設定
  const std::vector<int>& server_fds = server.getServerFds();
  for (size_t i = 0; i < server_fds.size(); ++i) {
    pollfd poll_fd;
    poll_fd.fd = server_fds[i];
    poll_fd.events = POLLIN;
    run_server.add_poll_fd(poll_fd);
  }

  std::cout << "Starting multiport server" << std::endl;
  run_server.runMultiPort(
      server);  // このメソッド名が正確に一致していることを確認

  // クリーンアップ（実行されることはないが念のため）
  server.closeSockets();

  return EXIT_SUCCESS;
}
