#include <dirent.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <fstream>

#include "../../srcs/HTTPResponse.hpp"
#include "../../srcs/ListenDirectory.hpp"

// テスト用の次のハンドラー
class MockHandler : public Handler {
 public:
  bool handlerCalled;

  MockHandler() : handlerCalled(false) {}
  void handleRequest(HTTPResponse& httpResponse) { handlerCalled = true; }
};

// テストフィクスチャ
class ListenDirectoryTest : public ::testing::Test {
 protected:
  std::string testDirPath;
  std::string subDirPath;
  std::string testFilePath1;
  std::string testFilePath2;
  std::string specialFilePath;

  void SetUp() override {
    // テスト用ディレクトリのセットアップ
    testDirPath = "./test_dir";
    subDirPath = testDirPath + "/subdir";
    testFilePath1 = testDirPath + "/file1.txt";
    testFilePath2 = testDirPath + "/file2.txt";
    specialFilePath = testDirPath + "/special file with spaces.txt";

    // テストディレクトリを作成
    mkdir(testDirPath.c_str(), 0755);
    mkdir(subDirPath.c_str(), 0755);

    // テストファイルを作成
    std::ofstream file1(testFilePath1.c_str());
    file1 << "This is test file 1" << std::endl;
    file1.close();

    std::ofstream file2(testFilePath2.c_str());
    file2 << "This is test file 2 with more content" << std::endl;
    file2 << "Second line" << std::endl;
    file2.close();

    std::ofstream specialFile(specialFilePath.c_str());
    specialFile << "File with special characters in name" << std::endl;
    specialFile.close();
  }

  void TearDown() override {
    // テスト用のファイルとディレクトリを削除
    if (access(testFilePath1.c_str(), F_OK) != -1) {
      remove(testFilePath1.c_str());
    }
    if (access(testFilePath2.c_str(), F_OK) != -1) {
      remove(testFilePath2.c_str());
    }
    if (access(specialFilePath.c_str(), F_OK) != -1) {
      remove(specialFilePath.c_str());
    }
    if (access(subDirPath.c_str(), F_OK) != -1) {
      rmdir(subDirPath.c_str());
    }
    if (access(testDirPath.c_str(), F_OK) != -1) {
      rmdir(testDirPath.c_str());
    }
  }

  // HTMLレスポンスに特定の文字列が含まれているかチェックするヘルパー関数
  bool responseContains(const std::string& response, const std::string& text) {
    return response.find(text) != std::string::npos;
  }
};

// 正常系: 有効なディレクトリに対するテスト
TEST_F(ListenDirectoryTest, ValidDirectory) {
  ListenDirectory listenDir(testDirPath);
  HTTPResponse response;

  listenDir.handleRequest(response);

  std::string body = response.getHttpResponseBody();

  // ボディが空でないことを確認
  EXPECT_FALSE(body.empty());

  // ディレクトリ名がタイトルに含まれているか
  EXPECT_TRUE(responseContains(body, "Directory listing for " + testDirPath));

  // すべてのファイルとディレクトリが表示されているか
  EXPECT_TRUE(responseContains(body, "file1.txt"));
  EXPECT_TRUE(responseContains(body, "file2.txt"));
  EXPECT_TRUE(responseContains(body, "special file with spaces.txt"));
  EXPECT_TRUE(responseContains(body, "subdir/"));

  // 親ディレクトリへのリンクがあるか
  EXPECT_TRUE(responseContains(body, "Parent Directory"));

  // Last ModifiedとSizeの列がないことを確認
  EXPECT_FALSE(responseContains(body, "Last Modified"));
  EXPECT_FALSE(responseContains(body, "Size"));
}

// 異常系: 存在しないディレクトリに対するテスト
TEST_F(ListenDirectoryTest, NonExistentDirectory) {
  ListenDirectory listenDir("./non_existent_directory");
  HTTPResponse response;

  listenDir.handleRequest(response);

  // ボディが空であることを確認
  EXPECT_TRUE(response.getHttpResponseBody().empty());
}

// 正常系: 空のディレクトリに対するテスト
TEST_F(ListenDirectoryTest, EmptyDirectory) {
  std::string emptyDir = "./empty_dir";
  mkdir(emptyDir.c_str(), 0755);

  ListenDirectory listenDir(emptyDir);
  HTTPResponse response;

  listenDir.handleRequest(response);

  std::string body = response.getHttpResponseBody();

  // ボディが空でないことを確認
  EXPECT_FALSE(body.empty());

  // 空ディレクトリでも親ディレクトリへのリンクがあるか
  EXPECT_TRUE(responseContains(body, "Parent Directory"));

  // テスト後にディレクトリを削除
  rmdir(emptyDir.c_str());
}

// ハンドラーチェーンのテスト: 次のハンドラーが呼ばれるか
TEST_F(ListenDirectoryTest, NextHandlerCalled) {
  ListenDirectory listenDir(testDirPath);
  MockHandler mockHandler;
  listenDir.setNextHandler(&mockHandler);

  HTTPResponse response;

  listenDir.handleRequest(response);

  // 次のハンドラーが呼ばれたか検証
  EXPECT_TRUE(mockHandler.handlerCalled);
}

// ハンドラーチェーンのテスト: 次のハンドラーがない場合
TEST_F(ListenDirectoryTest, NoNextHandler) {
  ListenDirectory listenDir(testDirPath);

  HTTPResponse response;

  // エラーなく実行できることを検証
  EXPECT_NO_THROW(listenDir.handleRequest(response));
  EXPECT_FALSE(response.getHttpResponseBody().empty());
}
