#include <gtest/gtest.h>

#include "ServerData.hpp"
#include <sys/wait.h>   // waitpid用
#include <cstring>      // strstr用

// ✅ ServerData のテストフィクスチャ
class ServerDataTest : public ::testing::Test {
 protected:
  ServerData* serverData;
  int client_fd;

  void SetUp() override {
    serverData = new ServerData();
    client_fd = -1;
  }

  void TearDown() override {
    if (client_fd != -1) {
      close(client_fd);
    }
    delete serverData;
  }
};

// ✅ set_server_fd() のテスト
TEST_F(ServerDataTest, SetServerFdCreatesValidSocket) {
  serverData->set_server_fd();
  int fd = serverData->get_server_fd();

  // ソケットが作成されたかチェック
  EXPECT_GT(fd, 0) << "Socket creation failed!";

  // ちゃんとソケットが有効かチェック
  int optval;
  socklen_t optlen = sizeof(optval);
  EXPECT_EQ(getsockopt(fd, SOL_SOCKET, SO_TYPE, &optval, &optlen), 0);
  EXPECT_EQ(optval, SOCK_STREAM);
}

// ✅ get_server_fd() の初期値テスト
TEST_F(ServerDataTest, DefaultServerFdIsInvalid) {
  EXPECT_EQ(serverData->get_server_fd(), -1);
}

// ✅ server_bind() のテスト
TEST_F(ServerDataTest, ServerBindSuccess) {
  // 実行前に既存のソケットをクリーンアップ
  system("fuser -k 8080/tcp >/dev/null 2>&1 || true");
  
  serverData->set_server_fd();

  // アドレス設定
  struct sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);
  serverData->set_new_socket(-1);  // 念のためリセット

  // `serverData` のアドレス設定を適用
  *(struct sockaddr_in*)&serverData->get_address() = address;

  // サーバーをバインド
  serverData->server_bind();

  // バインド成功を確認
  struct sockaddr_in bound_addr;
  socklen_t addr_len = sizeof(bound_addr);
  EXPECT_EQ(getsockname(serverData->get_server_fd(),
                      (struct sockaddr*)&bound_addr, &addr_len),
          0);
  EXPECT_EQ(bound_addr.sin_port, htons(8080));
}

// ✅ server_listen() のテスト
TEST_F(ServerDataTest, ServerListenSuccess) {
  serverData->set_server_fd();

  struct sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);
  *(struct sockaddr_in*)&serverData->get_address() = address;

  serverData->server_bind();
  serverData->server_listen();

  // サーバーがリスニング状態になっているかチェック
  int optval;
  socklen_t optlen = sizeof(optval);
  EXPECT_EQ(getsockopt(serverData->get_server_fd(), SOL_SOCKET, SO_ACCEPTCONN,
                       &optval, &optlen),
            0);
  EXPECT_EQ(optval, 1);
}

// ✅ server_accept() のテスト（クライアントを作成して接続）
TEST_F(ServerDataTest, ServerAcceptSuccess) {
  serverData->set_server_fd();

  struct sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);
  *(struct sockaddr_in*)&serverData->get_address() = address;

  serverData->server_bind();
  serverData->server_listen();

  // クライアントソケットを作成
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GT(client_fd, 0) << "Failed to create client socket";

  // クライアントがサーバーに接続
  ASSERT_EQ(connect(client_fd, (struct sockaddr*)&address, sizeof(address)), 0)
      << "Client connection failed";

  // サーバーがクライアントを受け入れる
  serverData->server_accept();
  EXPECT_GT(serverData->get_new_socket(), 0) << "Server accept failed";
}

// ✅ set_address_data() のテスト
TEST_F(ServerDataTest, SetAddressData) {
  serverData->set_address_data();
  struct sockaddr_in address = serverData->get_address();
  EXPECT_EQ(address.sin_family, AF_INET);
  EXPECT_EQ(address.sin_addr.s_addr, INADDR_ANY);
  EXPECT_EQ(address.sin_port, htons(8080));
}

// ✅ set_new_socket() のテスト
TEST_F(ServerDataTest, SetNewSocket) {
  serverData->set_new_socket(5);
  EXPECT_EQ(serverData->get_new_socket(), 5);
}
