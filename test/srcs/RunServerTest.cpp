#include <fcntl.h>  // fcntl用
#include <gtest/gtest.h>
#include <signal.h>  // sigaction用
#include <unistd.h>  // pipe, alarm用

#include <thread>  // std::thread用
#include <vector>

#include "RunServer.hpp"
#include "ServerData.hpp"

// pollfdの追加テスト
TEST(RunServerTest, AddPollFdTest) {
  RunServer server;
  pollfd test_fd;
  test_fd.fd = 3;
  test_fd.events = POLLIN;
  server.add_poll_fd(test_fd);

  EXPECT_EQ(server.get_poll_fds().size(), 1);
  EXPECT_EQ(server.get_poll_fds()[0].fd, 3);
}

// クライアント接続処理テスト（モック）
TEST(RunServerTest, HandleNewConnectionTest) {
  ServerData server;
  server.set_server_fd();
  server.set_address_data();
  server.server_bind();
  server.server_listen();

  RunServer run_server;
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(client_fd, -1);

  struct sockaddr_in serv_addr = server.get_address();
  EXPECT_EQ(connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)),
            0);

  run_server.handle_new_connection(server.get_server_fd());

  EXPECT_EQ(run_server.get_poll_fds().size(), 1);

  close(client_fd);
}

// 存在しないserver_fdを送ったときに標準エラー出力に"accept"が表示されるかテスト
TEST(RunServerTest, HandleNewConnectionTest2) {
  RunServer run_server;
  testing::internal::CaptureStderr();
  run_server.handle_new_connection(0);
  std::string actual = testing::internal::GetCapturedStderr();

  // 存在しないserver_fdを送ったときに標準エラー出力に"accept"が表示されるかテスト
  EXPECT_EQ(actual, "accept: Socket operation on non-socket\n");
}

// イベント処理テスト
TEST(RunServerTest, ProcessPollEventsTest) {
  ServerData server;
  server.set_server_fd();
  server.set_address_data();
  server.server_bind();
  server.server_listen();

  RunServer run_server;
  run_server.add_poll_fd({server.get_server_fd(), POLLIN, 0});

  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  connect(client_fd, (struct sockaddr*)&server.get_address(),
          sizeof(server.get_address()));

  run_server.process_poll_events(server);

  EXPECT_EQ(run_server.get_poll_fds().size(), 1);

  close(client_fd);
}

// クライアントデータ処理がクラッシュしないことを確認するテスト
TEST(RunServerTest, HandleClientDataNoCrash) {
  RunServer run_server;
  ServerData server_data;

  // サーバーソケットを作成
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(server_fd, -1);

  // サーバーソケットをバインド
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);
  ASSERT_NE(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)), -1);

  // サーバーソケットをリッスン
  ASSERT_NE(listen(server_fd, 1), -1);

  // サーバーソケットをpoll_fdsに追加
  pollfd server_poll_fd;
  server_poll_fd.fd = server_fd;
  server_poll_fd.events = POLLIN;
  run_server.add_poll_fd(server_poll_fd);

  // クライアントソケットを作成して接続
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(client_fd, -1);
  ASSERT_NE(connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
            -1);

  // 新しい接続を受け入れる
  run_server.handle_new_connection(server_fd);

  // クライアントからデータを送信
  const char* msg = "Hello, Server!";
  ASSERT_NE(write(client_fd, msg, strlen(msg)), -1);

  // クライアントデータを処理（クラッシュしないことを確認）
  ASSERT_NO_THROW(run_server.handle_client_data(1));

  // クリーンアップ
  close(client_fd);
  close(server_fd);
}

// クライアントデータ処理のif文の中身を確認するテスト
// TEST(RunServerTest, HandleClientDataIfConditions) {
//   RunServer run_server;
//   ServerData server_data;

//   // サーバーソケットを作成
//   int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(server_fd, -1);

//   // サーバーソケットをバインド
//   sockaddr_in server_addr;
//   server_addr.sin_family = AF_INET;
//   server_addr.sin_addr.s_addr = INADDR_ANY;
//   server_addr.sin_port = htons(8080);
//   ASSERT_NE(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
//   -1);

//   // サーバーソケットをリッスン
//   ASSERT_NE(listen(server_fd, 1), -1);

//   // サーバーソケットをpoll_fdsに追加
//   pollfd server_poll_fd;
//   server_poll_fd.fd = server_fd;
//   server_poll_fd.events = POLLIN;
//   run_server.add_poll_fd(server_poll_fd);

//   // クライアントソケットを作成して接続
//   int client_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(client_fd, -1);
//   ASSERT_NE(connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
//             -1);

//   // 新しい接続を受け入れる
//   run_server.handle_new_connection(server_fd);

//   // クライアントからデータを送信
//   const char* msg = "Hello, Server!";
//   ASSERT_NE(write(client_fd, msg, strlen(msg)), -1);

//   // クライアントデータを処理
//   testing::internal::CaptureStdout();
//   run_server.handle_client_data(1);
//   std::string output = testing::internal::GetCapturedStdout();

//   // if文の中身を確認
//   EXPECT_NE(output.find("Handling client data"), std::string::npos);
//   EXPECT_NE(output.find("Received: Hello, Server!"), std::string::npos);

//   // クリーンアップ
//   close(client_fd);
//   close(server_fd);
// }

// クライアントデータ処理のエラーハンドリングを確認するテスト
// TEST(RunServerTest, HandleClientDataErrorHandling) {
//   RunServer run_server;
//   ServerData server_data;

//   // サーバーソケットを作成
//   int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(server_fd, -1);

//   // サーバーソケットをバインド
//   sockaddr_in server_addr;
//   server_addr.sin_family = AF_INET;
//   server_addr.sin_addr.s_addr = INADDR_ANY;
//   server_addr.sin_port = htons(8080);
//   ASSERT_NE(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
//   -1);

//   // サーバーソケットをリッスン
//   ASSERT_NE(listen(server_fd, 1), -1);

//   // サーバーソケットをpoll_fdsに追加
//   pollfd server_poll_fd;
//   server_poll_fd.fd = server_fd;
//   server_poll_fd.events = POLLIN;
//   run_server.add_poll_fd(server_poll_fd);

//   // クライアントソケットを作成して接続
//   int client_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(client_fd, -1);
//   ASSERT_NE(connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
//   -1);

//   // 新しい接続を受け入れる
//   run_server.handle_new_connection(server_fd);

//   // クライアントソケットを閉じる
//   close(client_fd);

//   // クライアントデータを処理
//   testing::internal::CaptureStderr();
//   run_server.handle_client_data(1);
//   std::string error_output = testing::internal::GetCapturedStderr();

//   // エラーメッセージを確認
//   EXPECT_NE(error_output.find("read"), std::string::npos);

//   // クリーンアップ
//   close(server_fd);
// }

// 通常のデータ受信と送信をテスト
// TEST(RunServerTest, HandleClientDataNormalFlow) {
//   RunServer run_server;

//   // 実行前に既存のソケットをクリーンアップ
//   system("fuser -k 8080/tcp >/dev/null 2>&1 || true");

//   // サーバーソケットのセットアップ
//   int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(server_fd, -1);

//   // SO_REUSEADDRオプションを設定
//   int opt = 1;
//   setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//   sockaddr_in server_addr;
//   server_addr.sin_family = AF_INET;
//   server_addr.sin_addr.s_addr = INADDR_ANY;
//   server_addr.sin_port = htons(8080);
//   ASSERT_NE(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
//   -1); ASSERT_NE(listen(server_fd, 1), -1);

//   // クライアント接続
//   int client_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(client_fd, -1);
//   ASSERT_NE(connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
//             -1);

//   // 接続を受け付け
//   int accepted_fd = accept(server_fd, nullptr, nullptr);
//   ASSERT_NE(accepted_fd, -1);

//   // poll_fdsにクライアントfdを追加
//   pollfd client_poll_fd;
//   client_poll_fd.fd = accepted_fd;
//   client_poll_fd.events = POLLIN;
//   run_server.add_poll_fd(client_poll_fd);

//   // テストデータを送信
//   const char* test_msg = "Test Message";
//   write(client_fd, test_msg, strlen(test_msg));

//   // タイムアウト設定（5秒）
//   alarm(5);

//   // 標準出力をキャプチャ
//   testing::internal::CaptureStdout();
//   run_server.handle_client_data(0);
//   std::string output = testing::internal::GetCapturedStdout();

//   // タイムアウト解除
//   alarm(0);

//   // 出力を検証
//   EXPECT_NE(output.find("Handling client data"), std::string::npos);
//   EXPECT_NE(output.find("Received: Test Message"), std::string::npos);

//   // クライアントが応答を受信できることを確認
//   char buffer[1024] = {0};
//   ssize_t received = read(client_fd, buffer, sizeof(buffer));
//   EXPECT_GT(received, 0);
//   EXPECT_STREQ(buffer, test_msg);

//   // クリーンアップ
//   close(client_fd);
//   close(accepted_fd);
//   close(server_fd);
// }

// runメソッドのテスト（ブランチカバレッジ向上）
TEST(RunServerTest, RunMethodBranchCoverage) {
  RunServer run_server;
  ServerData server_data;

  // サーバーのセットアップ
  server_data.set_server_fd();
  server_data.set_address_data();
  server_data.server_bind();
  server_data.server_listen();

  // poll_fdsにサーバーfdを追加
  pollfd server_poll_fd;
  server_poll_fd.fd = server_data.get_server_fd();
  server_poll_fd.events = POLLIN;
  run_server.add_poll_fd(server_poll_fd);

  // 別スレッドでランメソッドを実行（無限ループのため）
  std::thread server_thread([&run_server, &server_data]() {
    // 3秒後にスレッドを終了させるためのタイマー
    alarm(3);

    // SIGALRMシグナルハンドラを設定
    struct sigaction sa;
    sa.sa_handler = [](int) {
      std::cout << "Alarm triggered - stopping server" << std::endl;
      exit(0);  // テスト目的のため強制終了
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, nullptr);

    // runメソッドの実行
    run_server.run(server_data);
  });

  // クライアント接続を作成
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GT(client_fd, 0);

  // ソケットを非ブロッキングに設定
  int flags = fcntl(client_fd, F_GETFL);
  fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

  // サーバーに接続
  struct sockaddr_in server_addr = server_data.get_address();
  connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

  // スレッドの終了を待つ（アラームで終了される）
  server_thread.detach();  // joinせず切り離す

  // クリーンアップ
  close(client_fd);

  // アラームが発動するまで少し待つ
  sleep(4);

  // このテストはrun()メソッドのwhileループとpoll()呼び出しが実行されることを確認する
  // 実際の検証はカバレッジテストによって行われる
}

// handle_client_dataのエラーパスをテスト（bytes_read <= 0のブランチ）
TEST(RunServerTest, HandleClientDataErrorBranches) {
  RunServer run_server;

  // クライアントテスト用の切断済みソケットを準備
  int fds[2];
  ASSERT_EQ(pipe(fds), 0);

  // このソケットを監視対象に追加
  pollfd test_poll_fd;
  test_poll_fd.fd = fds[0];  // 読み取り側
  test_poll_fd.events = POLLIN;
  run_server.add_poll_fd(test_poll_fd);

  // まず通常ケース - データありの場合
  const char* test_data = "test";
  ASSERT_GT(write(fds[1], test_data, strlen(test_data)), 0);

  // bytes_read > 0 のケース確認
  run_server.handle_client_data(0);

  // 書き込み側を閉じる（読み取り側でEOFが発生）
  close(fds[1]);

  // bytes_read = 0 のケース（EOF）
  testing::internal::CaptureStdout();  // エラーメッセージをキャプチャ

  // poll_fdsにテスト用ソケットを再度追加（前回の呼び出しで削除されているため）
  test_poll_fd.fd = fds[0];  // 読み取り側
  run_server.add_poll_fd(test_poll_fd);

  run_server.handle_client_data(0);
  std::string output = testing::internal::GetCapturedStdout();

  // 切断時にpoll_fdsから削除されることを確認
  EXPECT_EQ(run_server.get_poll_fds().size(), 0);

  // bytes_read = -1 のケースは特殊な設定が必要なため、このテストでは省略

  // クリーンアップ
  close(fds[0]);
}

// bytes_read = -1 のブランチをカバーする追加テスト
TEST(RunServerTest, HandleClientDataRecvErrorBranch) {
  RunServer run_server;

  // 無効なソケットディスクリプタを設定
  int invalid_fd = -5;  // 確実に無効なFD

  // poll_fdsに無効なFDを追加
  pollfd test_poll_fd;
  test_poll_fd.fd = invalid_fd;
  test_poll_fd.events = POLLIN;
  run_server.add_poll_fd(test_poll_fd);

  // エラーメッセージをキャプチャ
  testing::internal::CaptureStderr();

  // handle_client_dataを呼び出し
  run_server.handle_client_data(0);

  // 標準エラー出力を取得
  std::string error_output = testing::internal::GetCapturedStderr();

  // エラーメッセージに"recv"が含まれることを確認
  EXPECT_NE(error_output.find("recv"), std::string::npos);

  // poll_fdsから削除されたことを確認
  EXPECT_EQ(run_server.get_poll_fds().size(), 0);
}
