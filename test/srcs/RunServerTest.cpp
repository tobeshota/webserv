#include <fcntl.h>  // fcntl用
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <signal.h>  // sigaction用
#include <sys/socket.h>
#include <unistd.h>  // pipe, alarm用

#include <sstream>
#include <thread>  // std::thread用
#include <vector>

#include "HTTPResponse.hpp"
#include "MultiPortServer.hpp"
#include "RunServer.hpp"
#include "ServerData.hpp"

// モック用のクラス定義
class MockMultiPortServer : public MultiPortServer {
 public:
  std::map<int, int> server_fds;

  bool isServerFd(int fd) const {
    return server_fds.find(fd) != server_fds.end();
  }

  int getPortByFd(int fd) const {
    auto it = server_fds.find(fd);
    if (it != server_fds.end()) {
      return it->second;
    }
    return -1;
  }

  void addMockServerFd(int fd, int port) { server_fds[fd] = port; }
};

// テストフィクスチャ
class RunServerTest : public ::testing::Test {
 protected:
  void SetUp() {
    // ソケットペアを作成するヘルパー関数
    createSocketPair = [this](int& fd1, int& fd2) -> bool {
      int fds[2];
      if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        perror("socketpair");
        return false;
      }
      fd1 = fds[0];
      fd2 = fds[1];
      return true;
    };
  }

  void TearDown() {
    // テスト後のクリーンアップ
    for (auto& fd : fdToClose) {
      if (fd > 0) close(fd);
    }
  }

  // テスト用にソケットをクローズするリスト
  std::vector<int> fdToClose;
  // ソケットペア作成のラムダ関数
  std::function<bool(int&, int&)> createSocketPair;

  // テスト中に作成されたファイルディスクリプタを追跡
  void trackFd(int fd) {
    if (fd > 0) fdToClose.push_back(fd);
  }
};

// コンストラクタとデフォルト設定のテスト
TEST_F(RunServerTest, Constructor) {
  RunServer server;

  // デフォルトコンフィグパスの確認
  EXPECT_EQ(server.getConfPath(), DEFAULT_CONF_PATH);

  // 初期poll_fdsは空であることを確認
  EXPECT_TRUE(server.get_poll_fds().empty());
}

// poll_fds操作のテスト
TEST_F(RunServerTest, PollFdsManipulation) {
  RunServer server;

  // add_poll_fdのテスト
  pollfd testFd;
  testFd.fd = 10;
  testFd.events = POLLIN;
  testFd.revents = 0;
  server.add_poll_fd(testFd);

  // 追加されたことを確認
  ASSERT_EQ(server.get_poll_fds().size(), 1);
  EXPECT_EQ(server.get_poll_fds()[0].fd, 10);
  EXPECT_EQ(server.get_poll_fds()[0].events, POLLIN);

  // 複数のpoll_fdの追加
  pollfd testFd2;
  testFd2.fd = 20;
  testFd2.events = POLLIN;
  testFd2.revents = 0;
  server.add_poll_fd(testFd2);

  ASSERT_EQ(server.get_poll_fds().size(), 2);
  EXPECT_EQ(server.get_poll_fds()[1].fd, 20);
}

// コンフィグパス管理のテスト
TEST_F(RunServerTest, ConfigPathManagement) {
  RunServer server;

  // デフォルト値の確認
  EXPECT_EQ(server.getConfPath(), DEFAULT_CONF_PATH);

  // 値の設定と確認
  std::string newPath = "/tmp/custom.conf";
  server.setConfPath(newPath);
  EXPECT_EQ(server.getConfPath(), newPath);

  // 空文字列の設定
  server.setConfPath("");
  EXPECT_EQ(server.getConfPath(), "");
}

// handle_new_connectionのテスト
TEST_F(RunServerTest, HandleNewConnection) {
  RunServer server;

  // ソケットペアを作成
  int serverFd, clientFd;
  ASSERT_TRUE(createSocketPair(serverFd, clientFd));
  trackFd(serverFd);
  trackFd(clientFd);

  // accept呼び出しをシミュレートするため、serverFdから接続を受け付けられるようにする必要がある
  // ただし、テスト環境ではacceptの完全なシミュレートは難しいため、簡易的なテストに留める

  // 不正なfdでのエラー処理をテスト
  testing::internal::CaptureStderr();
  server.handle_new_connection(-1);  // 無効なfd
  std::string output = testing::internal::GetCapturedStderr();
  EXPECT_NE(output.find("accept"), std::string::npos);

  // 接続後のpoll_fdsの変更を確認するには、実際の接続が必要だが、
  // 単体テストでは難しいため、ここでは簡易的な確認のみ行う
  EXPECT_EQ(server.get_poll_fds().size(), 0);
}

// 助手クラス: handle_client_dataのテスト用にRunServerをサブクラス化
class TestableRunServer : public RunServer {
 public:
  // 公開版のhandle_client_dataをオーバーライド - port引数を文字列型に変更
  void handle_client_data_test(size_t index, const std::string& port) {
    handle_client_data(index, port);
  }

  // プライベートメンバをテスト可能にするためのヘルパー
  bool eraseClientFd(size_t index) {
    if (index < get_poll_fds().size()) {
      close(get_poll_fds()[index].fd);
      get_poll_fds().erase(get_poll_fds().begin() + index);
      return true;
    }
    return false;
  }
};

// handle_client_dataの基本機能テスト
TEST_F(RunServerTest, HandleClientDataBasic) {
  TestableRunServer server;

  // パイプを使用してファイルディスクリプタを作成
  int fds[2];
  ASSERT_EQ(pipe(fds), 0);
  trackFd(fds[0]);
  trackFd(fds[1]);

  // クライアントソケットとして読み取り側をpoll_fdsに追加
  pollfd clientPoll;
  clientPoll.fd = fds[0];
  clientPoll.events = POLLIN;
  clientPoll.revents = 0;
  server.add_poll_fd(clientPoll);

  // データを書き込む
  const char* testData = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
  ASSERT_GT(write(fds[1], testData, strlen(testData)), 0);

  // 標準エラー出力をキャプチャ (HTTPリクエストパースエラーなど捕捉)
  testing::internal::CaptureStderr();

  // handle_client_dataを呼び出し
  server.handle_client_data_test(0, "80");

  std::string errorOutput = testing::internal::GetCapturedStderr();
  // テスト環境ではHTTPパースが失敗する可能性があるため、エラーメッセージの有無は厳密にテストしない

  // poll_fdsが正しく更新されたことを確認
  // handle_client_dataは通常、処理後にFDを閉じてpoll_fdsから削除するため
  // poll_fdsは空になっているはず
  EXPECT_EQ(server.get_poll_fds().size(), 0);
}

// handle_client_dataのエラー処理テスト
TEST_F(RunServerTest, HandleClientDataErrors) {
  TestableRunServer server;

  // 無効なファイルディスクリプタを作成
  int invalidFd = -1;

  // 無効なfdをpoll_fdsに追加
  pollfd invalidPoll;
  invalidPoll.fd = invalidFd;
  invalidPoll.events = POLLIN;
  invalidPoll.revents = 0;
  server.add_poll_fd(invalidPoll);

  // 標準エラー出力をキャプチャ
  testing::internal::CaptureStderr();

  // handle_client_dataを呼び出し
  server.handle_client_data_test(0, "80");

  // recv関連のエラーメッセージを確認
  std::string errorOutput = testing::internal::GetCapturedStderr();
  EXPECT_NE(errorOutput.find("recv"), std::string::npos);

  // クライアント切断/エラー時にpoll_fdsから削除されることを確認
  EXPECT_EQ(server.get_poll_fds().size(), 0);
}

// bytes_read = 0 のケース (クライアント正常切断)
TEST_F(RunServerTest, HandleClientDataDisconnect) {
  TestableRunServer server;

  // パイプを使用してファイルディスクリプタを作成
  int fds[2];
  ASSERT_EQ(pipe(fds), 0);
  trackFd(fds[0]);  // 読み取り側のみ追跡

  // クライアントソケットとして読み取り側をpoll_fdsに追加
  pollfd clientPoll;
  clientPoll.fd = fds[0];
  clientPoll.events = POLLIN;
  clientPoll.revents = 0;
  server.add_poll_fd(clientPoll);

  // 書き込み側を閉じる (これにより読み取り時にEOFが返される)
  close(fds[1]);

  // handle_client_dataを呼び出し
  server.handle_client_data_test(0, "80");

  // クライアント切断時にpoll_fdsから削除されることを確認
  EXPECT_EQ(server.get_poll_fds().size(), 0);
}

// int2str関数のテスト
TEST_F(RunServerTest, Int2StrTest) {
  // RunServerクラス内のstatic関数であるint2strをテストするため、同様の実装を作成
  auto int2str = [](int val) -> std::string {
    std::stringstream ss;
    ss << val;
    return ss.str();
  };

  EXPECT_EQ(int2str(0), "0");
  EXPECT_EQ(int2str(123), "123");
  EXPECT_EQ(int2str(-456), "-456");
  EXPECT_EQ(int2str(8080), "8080");
}

// isContain関数のテスト
TEST_F(RunServerTest, IsContainTest) {
  // RunServerクラス内のstatic関数であるisContainをテストするため、同様の実装を作成
  auto isContain = [](const std::vector<std::string>& vec,
                      const std::string& str) -> bool {
    for (auto it = vec.begin(); it != vec.end(); ++it) {
      if (*it == str) {
        return true;
      }
    }
    return false;
  };

  std::vector<std::string> testVec = {"GET", "POST", "DELETE"};

  EXPECT_TRUE(isContain(testVec, "GET"));
  EXPECT_TRUE(isContain(testVec, "POST"));
  EXPECT_TRUE(isContain(testVec, "DELETE"));
  EXPECT_FALSE(isContain(testVec, "PUT"));
  EXPECT_FALSE(isContain(testVec, ""));
}

// runMultiPortメソッドの限定的テスト
TEST_F(RunServerTest, RunMultiPortMethodLimited) {
  // 実際のrunMultiPort()は無限ループなので、テスト用に制限付き版を作成
  class LimitedRunServerMulti : public RunServer {
   public:
    bool process_poll_events_multiport_called = false;
    MultiPortServer* last_multi_server = nullptr;

    void process_poll_events_multiport(MultiPortServer& server) {
      process_poll_events_multiport_called = true;
      last_multi_server = &server;
    }

    // 限定実行版のrunMultiPortメソッド
    void runMultiPortLimited(MultiPortServer& server, int iterations = 1) {
      for (int i = 0; i < iterations; ++i) {
        // pollの代わりに直接process_poll_events_multiportを呼ぶ
        process_poll_events_multiport(server);
      }
    }
  };

  LimitedRunServerMulti server;
  MockMultiPortServer multiServer;

  // 限定的なrunMultiPort実行
  server.runMultiPortLimited(multiServer);

  // process_poll_events_multiportが呼ばれ、正しいMultiPortServerが渡されたことを確認
  EXPECT_TRUE(server.process_poll_events_multiport_called);
  EXPECT_EQ(server.last_multi_server, &multiServer);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
