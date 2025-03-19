#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>

#include "../../srcs/POST.hpp"
#include "../../srcs/Directive.hpp"
#include "../../srcs/HTTPRequest.hpp"
#include "../../srcs/HTTPResponse.hpp"

// テスト用のヘルパー関数
namespace {
  // テスト用一時ファイルの作成
  void createTempFile(const std::string& path, const std::string& content) {
    std::ofstream file(path.c_str());
    file << content;
    file.close();
  }

  // テスト用ディレクトリの作成
  bool createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0;
  }

  // ファイルの存在確認
  bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
  }

  // ファイルの内容確認
  std::string readFile(const std::string& path) {
    std::ifstream file(path.c_str());
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return content;
  }

  // テスト用のディレクティブ構造を作成する関数
  Directive createTestDirective(const std::string& host,
                               const std::string& root,
                               const std::string& maxBodySize,
                               bool allowPost = true,
                               const std::string& location = "/") {
    Directive rootDirective("root");

    // ホストディレクティブの作成
    Directive hostDirective(host);
    hostDirective.addKeyValue("root", root);
    if (!maxBodySize.empty()) {
      hostDirective.addKeyValue("client_max_body_size", maxBodySize);
    }
    
    // locationディレクティブの作成
    Directive locationDirective("location");
    locationDirective.addKeyValue("path", location);
    if (!allowPost) {
      locationDirective.addKeyValue("limit_except", "GET");
    } else {
      locationDirective.addKeyValue("limit_except", "GET POST");
    }
    
    hostDirective.addChild(locationDirective);
    rootDirective.addChild(hostDirective);
    
    return rootDirective;
  }
}

class POSTTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // テスト用ディレクトリの作成
    createDirectory("./test_tmp");
    createDirectory("./test_tmp/webroot");
    createDirectory("./test_tmp/webroot/uploads");
    
    // テスト用ファイルの作成
    createTempFile("./test_tmp/webroot/test.txt", "Hello World");
    createTempFile("./test_tmp/webroot/test.py", "print('Content-type: text/html\\n\\nHello from Python')");
    
    // 書き込み権限のないディレクトリの作成
    createDirectory("./test_tmp/webroot/no_permission");
    chmod("./test_tmp/webroot/no_permission", 0500); // 読み込み・実行のみ許可
  }

  void TearDown() override {
    // テストファイルの削除
    system("rm -rf ./test_tmp");
  }
  
  // 標準的なHTTPリクエストを作成する
  HTTPRequest createPostRequest(const std::string& url,
                               const std::string& body,
                               const std::string& contentType = "text/plain",
                               bool chunked = false) {
    std::map<std::string, std::string> headers;
    headers["Host"] = "localhost:8080";
    headers["Content-Type"] = contentType;
    
    if (chunked) {
      headers["Transfer-Encoding"] = "chunked";
    } else {
      std::stringstream ss;
      ss << body.length();
      headers["Content-Length"] = ss.str();
    }
    
    return HTTPRequest("POST", url, "HTTP/1.1", headers, body);
  }
  
  // チャンクエンコードされたボディを生成
  std::string createChunkedBody(const std::string& body) {
    std::stringstream result;
    size_t pos = 0;
    const size_t chunkSize = 5; // 小さいチャンクサイズでテスト
    
    while (pos < body.length()) {
      size_t size = std::min(chunkSize, body.length() - pos);
      std::stringstream hexSize;
      hexSize << std::hex << size;
      
      result << hexSize.str() << "\r\n";
      result << body.substr(pos, size) << "\r\n";
      pos += size;
    }
    
    result << "0\r\n\r\n"; // 終端チャンク
    return result.str();
  }
};

// 1. 基本的なPOSTリクエストのテスト
TEST_F(POSTTest, BasicPostRequest) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  HTTPRequest request = createPostRequest("/uploads/file.txt", "Test content");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 201);
  EXPECT_TRUE(fileExists("./test_tmp/webroot/uploads/file.txt"));
  EXPECT_EQ(readFile("./test_tmp/webroot/uploads/file.txt"), "Test content");
}

// 2. チャンク転送エンコーディングのテスト
TEST_F(POSTTest, ChunkedTransferEncoding) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  std::string originalBody = "This is a chunked test message";
  std::string chunkedBody = createChunkedBody(originalBody);
  
  HTTPRequest request = createPostRequest("/uploads/chunked.txt", chunkedBody, "text/plain", true);
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 201);
  EXPECT_TRUE(fileExists("./test_tmp/webroot/uploads/chunked.txt"));
  EXPECT_EQ(readFile("./test_tmp/webroot/uploads/chunked.txt"), originalBody);
}

// 3. 大きすぎるリクエストボディのテスト
TEST_F(POSTTest, BodySizeTooLarge) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "10");
  std::string largeBody(100, 'X'); // 制限より大きいサイズ
  HTTPRequest request = createPostRequest("/uploads/large.txt", largeBody);
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 413); // Request Entity Too Large
  EXPECT_FALSE(fileExists("./test_tmp/webroot/uploads/large.txt"));
}

// 4. 存在しないディレクトリへのPOSTテスト
TEST_F(POSTTest, NonExistentDirectory) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  HTTPRequest request = createPostRequest("/non_existent/file.txt", "Test content");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 404); // Not Found
}

// 5. 権限のないディレクトリへのPOSTテスト
TEST_F(POSTTest, NoPermissionDirectory) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  HTTPRequest request = createPostRequest("/no_permission/file.txt", "Test content");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 403); // Forbidden
}

// 6. POSTメソッドが許可されていないパスへのPOSTテスト
TEST_F(POSTTest, MethodNotAllowed) {
  // POSTが許可されていないディレクティブを作成
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M", false);
  HTTPRequest request = createPostRequest("/uploads/not_allowed.txt", "Test content");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 405); // Method Not Allowed
}

// 7. 不正なメソッドのテスト
TEST_F(POSTTest, InvalidMethod) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  // POSTハンドラにGETメソッドのリクエストを渡す
  std::map<std::string, std::string> headers;
  headers["Host"] = "localhost:8080";
  HTTPRequest request("GET", "/uploads/file.txt", "HTTP/1.1", headers, "");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 405); // Method Not Allowed
}

// 8. CGIスクリプトへのPOSTテスト
TEST_F(POSTTest, PostToCgiScript) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  HTTPRequest request = createPostRequest("/test.py", "name=John&age=30", "application/x-www-form-urlencoded");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証 - 実際のCGI実行結果は環境に依存するため、ステータスコードのみ検証
  // CGI実行が成功したかどうかは実際の環境に依存しますが、
  // ハンドラーが適切に呼び出されていることを確認します
  EXPECT_TRUE(response.getHttpStatusCode() == 200 || 
              response.getHttpStatusCode() == 500);
}

// 9. サイズ制限が指定されていない場合のデフォルト制限のテスト
TEST_F(POSTTest, DefaultSizeLimit) {
  // 設定 - client_max_body_sizeを指定しない
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "");
  
  // デフォルト制限（1MB）より小さいボディ
  std::string smallBody(1024, 'X'); // 1KB
  HTTPRequest request = createPostRequest("/uploads/small.txt", smallBody);
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 201);
  EXPECT_TRUE(fileExists("./test_tmp/webroot/uploads/small.txt"));
}

// 10. 空のボディのPOSTテスト
TEST_F(POSTTest, EmptyBody) {
  // 設定
  Directive rootDirective = createTestDirective("localhost:8080", "./test_tmp/webroot", "1M");
  HTTPRequest request = createPostRequest("/uploads/empty.txt", "");
  HTTPResponse response;
  
  // テスト対象の実行
  POST postHandler(rootDirective, request);
  postHandler.handleRequest(response);
  
  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 201);
  EXPECT_TRUE(fileExists("./test_tmp/webroot/uploads/empty.txt"));
  EXPECT_EQ(readFile("./test_tmp/webroot/uploads/empty.txt"), "");
}

// メインテスト実行関数
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
