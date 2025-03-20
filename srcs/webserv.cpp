#include <cstdlib>
#include <iostream>
#include <vector>

#include "MultiPortServer.hpp"
#include "OSInit.hpp"
#include "RunServer.hpp"  // 明示的にインクルード
#include "TOMLParser.hpp"

int webserv(int argc, char** argv) {
  (void)argc;
  (void)argv;

  RunServer run_server;

  // TOMLParser toml_parser;
  // Directive* directive = toml_parser.parseFromFile("./conf/webserv.conf");

  // 複数のポートを用意（本来はconfファイルから取得）
  std::vector<int> ports;
  ports.push_back(8080);
  ports.push_back(8081);
  ports.push_back(8082);

  // マルチポートサーバーを作成
  MultiPortServer server;
  server.setPorts(ports);

  // ソケット初期化
  if (!server.initializeSockets()) {
    std::cerr << "サーバーソケットの初期化に失敗しました" << std::endl;
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
  std::cout << "複数ポート対応サーバー起動中..." << std::endl;
  run_server.runMultiPort(
      server);  // このメソッド名が正確に一致していることを確認

  // クリーンアップ（実行されることはないが念のため）
  server.closeSockets();

  return EXIT_SUCCESS;
}
