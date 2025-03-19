#include <gtest/gtest.h>

#include <cstdio>  // std::removeのため
#include <fstream>

#include "DeleteClientMethod.hpp"

// ファイルが存在するか確認する関数
bool fileExists(const std::string& filename) {
  std::ifstream file(filename.c_str());
  return file.good();
}

TEST(DeleteClientMethodTest, SuccessfulDeletion) {
  // Setup
  std::string testFile = "test_delete.txt";
  std::ofstream file(testFile.c_str());
  file << "Test content";
  file.close();

  // ファイルが作成されたことを確認
  ASSERT_TRUE(fileExists(testFile));

  HTTPRequest req("DELETE", testFile, "HTTP/1.1",
                  std::map<std::string, std::string>(), "");
  HTTPResponse response;
  DeleteClientMethod handler(req);

  // Execute
  handler.handleRequest(response);

  // Verify
  EXPECT_EQ(response.getHttpStatusCode(), 200);
  EXPECT_FALSE(fileExists(testFile));
}

TEST(DeleteClientMethodTest, FileNotFound) {
  // Setup
  std::string nonExistentFile = "nonexistent_file.txt";
  // 確実にファイルが存在しないことを確認
  if (fileExists(nonExistentFile)) {
    std::remove(nonExistentFile.c_str());
  }

  HTTPRequest req("DELETE", nonExistentFile, "HTTP/1.1",
                  std::map<std::string, std::string>(), "");
  HTTPResponse response;
  DeleteClientMethod handler(req);

  // Execute
  handler.handleRequest(response);

  // Verify
  EXPECT_EQ(response.getHttpStatusCode(), 404);
}

TEST(DeleteClientMethodTest, ChainNextHandler) {
  // Setup
  HTTPRequest req("DELETE", "test.txt", "HTTP/1.1",
                  std::map<std::string, std::string>(), "");
  HTTPResponse response;
  DeleteClientMethod handler(req);

  class MockHandler : public Handler {
   public:
    bool called = false;
    void handleRequest(HTTPResponse&) { called = true; }
  };

  MockHandler* mockNext = new MockHandler();
  handler.setNextHandler(mockNext);

  // Execute
  handler.handleRequest(response);

  // Verify
  EXPECT_TRUE(mockNext->called);
  delete mockNext;
}

TEST(DeleteClientMethodTest, EmptyURL) {
  // Setup
  HTTPRequest req("DELETE", "", "HTTP/1.1",
                  std::map<std::string, std::string>(), "");
  HTTPResponse response;
  DeleteClientMethod handler(req);

  // Execute
  handler.handleRequest(response);

  // Verify
  EXPECT_EQ(response.getHttpStatusCode(), 404);
}

TEST(DeleteClientMethodTest, InvalidFilePath) {
  // Setup
  HTTPRequest req("DELETE", "/invalid/path/that/should/not/exist/*", "HTTP/1.1",
                  std::map<std::string, std::string>(), "");
  HTTPResponse response;
  DeleteClientMethod handler(req);

  // Execute
  handler.handleRequest(response);

  // Verify
  EXPECT_EQ(response.getHttpStatusCode(), 404);
}