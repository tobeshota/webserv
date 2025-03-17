#include <gtest/gtest.h>
#include "PrintResponse.hpp"
#include "HTTPResponse.hpp"
#include <fcntl.h>
#include <fstream>

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

TEST_F(PrintResponseTest, HandleRequestTest) {
    PrintResponse printer(mockSocket[0]);
    HTTPResponse response;
    
    // テストファイル作成
    const char* test_file = "/tmp/test_response.txt";
    const char* test_content = "Hello, World!";
    std::ofstream ofs(test_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << test_content;
    ofs.close();

    // HTTPResponseの設定
    response.setHttpStatusCode(200);
    response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
    response.setHttpResponseHeader("Content-Type: text/plain\r\nContent-Length: 13\r\n\r\n");
    response.setHttpResponseBody(test_file);

    EXPECT_NO_THROW(printer.handleRequest(response));

    // レスポンスの確認
    char buffer[2048] = {0};
    ssize_t received = read(mockSocket[1], buffer, sizeof(buffer));
    ASSERT_GT(received, 0);
    
    std::string response_str(buffer, received);
    EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
    EXPECT_TRUE(response_str.find("Content-Type: text/plain") != std::string::npos);
    EXPECT_TRUE(response_str.find("Hello, World!") != std::string::npos);

    unlink(test_file);
}

TEST_F(PrintResponseTest, HandleRequestEmptyBodyTest) {
    PrintResponse printer(mockSocket[0]);
    HTTPResponse response;
    
    response.setHttpStatusCode(204);
    response.setHttpStatusLine("HTTP/1.1 204 No Content\r\n");
    response.setHttpResponseHeader("Server: webserv\r\n\r\n");
    response.setHttpResponseBody("");

    EXPECT_NO_THROW(printer.handleRequest(response));

    char buffer[1024];
    ssize_t received = read(mockSocket[1], buffer, sizeof(buffer));
    ASSERT_GT(received, 0);
    
    std::string response_str(buffer, received);
    EXPECT_TRUE(response_str.find("HTTP/1.1 204 No Content") != std::string::npos);
    EXPECT_TRUE(response_str.find("Server: webserv") != std::string::npos);
}

TEST_F(PrintResponseTest, HandleRequestInvalidFileTest) {
    PrintResponse printer(mockSocket[0]);
    HTTPResponse response;
    
    response.setHttpStatusCode(200);
    response.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
    response.setHttpResponseHeader("Content-Type: text/plain\r\n\r\n");
    response.setHttpResponseBody("/nonexistent/file.txt");

    EXPECT_THROW(printer.handleRequest(response), std::runtime_error);
}

TEST_F(PrintResponseTest, AsshukuTest) {
    // テストファイル作成
    const char* test_file = "/tmp/test_asshuku.txt";
    std::string test_content(3072, 'X');  // 3KBのデータ
    
    std::ofstream ofs(test_file);
    ASSERT_TRUE(ofs.is_open());
    ofs << test_content;
    ofs.close();

    int fd = open(test_file, O_RDONLY);
    ASSERT_GT(fd, 0);

    std::vector<std::string> chunks;
    EXPECT_NO_THROW(chunks = PrintResponse::asshuku(fd));
    
    // 3072バイトは1024バイトで3分割されるはず
    EXPECT_EQ(chunks.size(), 3);
    for (const auto& chunk : chunks) {
        EXPECT_EQ(chunk.size(), 1024);
    }

    close(fd);
    unlink(test_file);
}

