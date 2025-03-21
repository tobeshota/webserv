#include <cstdlib>
#include <iostream>
#include <vector>

#include "MultiPortServer.hpp"
#include "OSInit.hpp"
#include "RunServer.hpp"  // 明示的にインクルード
#include "TOMLParser.hpp"

//// 複数のポートを用意（本来はconfファイルから取得）の関数を作る
std::vector<int> getPorts() {
  std::vector<int> ports;
  TOMLParser toml_parser;
  Directive* directive = NULL;

  try {
    directive = toml_parser.parseFromFile("./conf/webserv.conf");
    if (directive == NULL) {
      std::cerr << "Failed to parse configuration file" << std::endl;
      return ports;
    }

    // ポート番号を取得
    std::vector<std::string> listen_directives = directive->getValues("listen");
    std::cout << "listen_directives.size() " << listen_directives.size()
              << std::endl;
    for (size_t i = 0; i < listen_directives.size(); ++i) {
      std::string port_str = listen_directives[i];
      int port = std::stoi(port_str);
      ports.push_back(port);
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception in getPorts: " << e.what() << std::endl;
  }

  delete directive;
  return ports;
}

int webserv(int argc, char** argv) {
  (void)argc;
  (void)argv;

  RunServer run_server;
  std::vector<int> ports = getPorts();

  // マルチポートサーバーを作成
  MultiPortServer server;
  server.setPorts(ports);

  // OSInitを使用してソケット初期化
  OSInit osInit;
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

  // イベントループ開始（メソッド名が正確に一致していることを確認）
  std::cout << "Starting multiport server" << std::endl;
  run_server.runMultiPort(
      server);  // このメソッド名が正確に一致していることを確認

  // クリーンアップ（実行されることはないが念のため）
  server.closeSockets();

  return EXIT_SUCCESS;
}
