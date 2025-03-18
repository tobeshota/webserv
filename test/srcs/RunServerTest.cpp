#include <gtest/gtest.h>

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

// // クライアントデータ処理がクラッシュしないことを確認するテスト
// TEST(RunServerTest, HandleClientDataNoCrash) {
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

//   // クライアントデータを処理（クラッシュしないことを確認）
//   ASSERT_NO_THROW(run_server.handle_client_data(1));

//   // クリーンアップ
//   close(client_fd);
//   close(server_fd);
// }

// // クライアントデータ処理のif文の中身を確認するテスト
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

// // 通常のデータ受信と送信をテスト
// TEST(RunServerTest, HandleClientDataNormalFlow) {
//   RunServer run_server;

//   // サーバーソケットのセットアップ
//   int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(server_fd, -1);

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

//   // 標準出力をキャプチャ
//   testing::internal::CaptureStdout();
//   run_server.handle_client_data(0);
//   std::string output = testing::internal::GetCapturedStdout();

//   // 出力を検証
//   EXPECT_NE(output.find("Handling client data"), std::string::npos);
//   EXPECT_NE(output.find("Received: Test Message"), std::string::npos);

//   // 接続が維持されていることを確認
//   EXPECT_EQ(run_server.get_poll_fds().size(), 1);

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

// // クライアント切断のテスト
// TEST(RunServerTest, HandleClientDataDisconnect) {
//   RunServer run_server;

//   // サーバーソケットのセットアップ
//   int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//   ASSERT_NE(server_fd, -1);

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

//   // クライアントを切断
//   close(client_fd);

//   // 標準出力をキャプチャ
//   testing::internal::CaptureStdout();
//   run_server.handle_client_data(0);
//   std::string output = testing::internal::GetCapturedStdout();

//   // 切断メッセージを確認
//   EXPECT_NE(output.find("Client disconnected"), std::string::npos);

//   // poll_fdsから削除されていることを確認
//   EXPECT_EQ(run_server.get_poll_fds().size(), 0);

//   // クリーンアップ
//   close(accepted_fd);
//   close(server_fd);
// }

// 書き込みエラーのテスト
TEST(RunServerTest, HandleClientDataWriteError) {
  RunServer run_server;

  // サーバーソケットのセットアップ
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(server_fd, -1);

  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);
  ASSERT_NE(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)), -1);
  ASSERT_NE(listen(server_fd, 1), -1);

  // クライアント接続
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(client_fd, -1);
  ASSERT_NE(connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)),
            -1);

  // 接続を受け付け
  int accepted_fd = accept(server_fd, nullptr, nullptr);
  ASSERT_NE(accepted_fd, -1);

  // poll_fdsに無効なfdを追加（書き込みエラーを発生させる）
  pollfd client_poll_fd;
  client_poll_fd.fd = -1;  // 無効なfd
  client_poll_fd.events = POLLIN;
  run_server.add_poll_fd(client_poll_fd);

  // 標準エラー出力をキャプチャ
  testing::internal::CaptureStderr();
  run_server.handle_client_data(0);
  std::string error_output = testing::internal::GetCapturedStderr();

  // エラーメッセージを確認
  EXPECT_NE(error_output.find("read"), std::string::npos);

  // poll_fdsから削除されていることを確認
  EXPECT_EQ(run_server.get_poll_fds().size(), 0);

  // クリーンアップ
  close(client_fd);
  close(accepted_fd);
  close(server_fd);
}