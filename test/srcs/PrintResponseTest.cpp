#include <fcntl.h>
#include <gtest/gtest.h>

#include <fstream>

#include "HTTPResponse.hpp"
#include "PrintResponse.hpp"

class PrintResponseTest : public ::testing::Test {
 protected:
  int mockSocket[2];

  virtual void SetUp() override {
    // テスト用のソケットペアを作成
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, mockSocket), 0);
  }

  virtual void TearDown() override {
    close(mockSocket[0]);
    close(mockSocket[1]);
  }
};

// TEST_F(PrintResponseTest, HandleRequestTest) {
//   PrintResponse printer(mockSocket[0]);
//   HTTPResponse response;

//   // テストファイル作成
//   const char* test_file = "/tmp/test_response.txt";
//   const char* test_content = "Hello, World!";
//   std::ofstream ofs(test_file);
//   ASSERT_TRUE(ofs.is_open());
//   ofs << test_content;
//   ofs.close();

//   // HTTPResponseの設定
//   response.setHttpStatusCode(200);
//   response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
//   response.setHttpResponseHeader(
//       "Content-Type: text/plain\r\nContent-Length: 13\r\n\r\n");
//   response.setHttpResponseBody(test_file);

//   EXPECT_NO_THROW(printer.handleRequest(response));

//   // レスポンスの確認
//   char buffer[2048] = {0};
//   ssize_t received = read(mockSocket[1], buffer, sizeof(buffer));
//   ASSERT_GT(received, 0);

//   std::string response_str(buffer, received);
//   EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
//   EXPECT_TRUE(response_str.find("Content-Type: text/plain") !=
//               std::string::npos);
//   EXPECT_TRUE(response_str.find("Hello, World!") != std::string::npos);

//   unlink(test_file);
// }

// TEST_F(PrintResponseTest, HandleRequestEmptyBodyTest) {
//   PrintResponse printer(mockSocket[0]);
//   HTTPResponse response;

//   response.setHttpStatusCode(204);
//   response.setHttpStatusLine("HTTP/1.1 204 No Content\r\n");
//   response.setHttpResponseHeader("Server: webserv\r\n\r\n");
//   response.setHttpResponseBody("");

//   EXPECT_NO_THROW(printer.handleRequest(response));

//   char buffer[1024];
//   ssize_t received = read(mockSocket[1], buffer, sizeof(buffer));
//   ASSERT_GT(received, 0);

//   std::string response_str(buffer, received);
//   EXPECT_TRUE(response_str.find("HTTP/1.1 204 No Content") !=
//               std::string::npos);
//   EXPECT_TRUE(response_str.find("Server: webserv") != std::string::npos);
// }

// TEST_F(PrintResponseTest, HandleRequestInvalidFileTest) {
//   PrintResponse printer(mockSocket[0]);
//   HTTPResponse response;

//   response.setHttpStatusCode(200);
//   response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
//   response.setHttpResponseHeader("Content-Type: text/plain\r\n\r\n");
//   response.setHttpResponseBody("/nonexistent/file.txt");

//   EXPECT_THROW(printer.handleRequest(response), std::runtime_error);
// }

// 状態ラインの送信失敗テスト
TEST_F(PrintResponseTest, HandleRequestFailStatusLineTest) {
  PrintResponse printer(-1);  // 無効なソケット
  HTTPResponse response;

  response.setHttpStatusCode(200);
  response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");

  EXPECT_THROW(
      {
        try {
          printer.handleRequest(response);
        } catch (const std::runtime_error& e) {
          EXPECT_STREQ("Failed to send status line", e.what());
          throw;
        }
      },
      std::runtime_error);
}

// // ヘッダーの送信失敗テスト
// TEST_F(PrintResponseTest, HandleRequestFailHeaderTest) {
//     PrintResponse printer(mockSocket[0]);
//     HTTPResponse response;

//     response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
//     response.setHttpResponseHeader("Content-Type: text/plain\r\n\r\n");

//     // ステータスライン送信後にソケットを閉じる
//     ASSERT_GT(send(mockSocket[0], response.getHttpStatusLine().c_str(),
//                    response.getHttpStatusLine().size(), MSG_NOSIGNAL), 0);
//     close(mockSocket[0]);

//     EXPECT_THROW({
//         try {
//             printer.handleRequest(response);
//         } catch (const std::runtime_error& e) {
//             EXPECT_STREQ("Failed to send response header", e.what());
//             throw;
//         }
//     }, std::runtime_error);
// }

// ボディの送信失敗テスト
TEST_F(PrintResponseTest, HandleRequestFailBodyTest) {
  PrintResponse printer(mockSocket[0]);
  HTTPResponse response;

  // 存在しないファイルパスを設定
  response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
  response.setHttpResponseHeader("Content-Type: text/plain\r\n\r\n");
  response.setHttpResponseBody("/nonexistent/file/path.txt");

  EXPECT_THROW(
      {
        try {
          printer.handleRequest(response);
        } catch (const std::runtime_error& e) {
          EXPECT_STREQ("Failed to open response body file", e.what());
          throw;
        }
      },
      std::runtime_error);
}
