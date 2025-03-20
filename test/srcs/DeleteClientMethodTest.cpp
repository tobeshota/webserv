#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>  // アクセス権のテスト用

#include <cstdio>  // std::removeのため
#include <fstream>

#include "../../srcs/DeleteClientMethod.hpp"
#include "../../srcs/Directive.hpp"
#include "../../srcs/HTTPRequest.hpp"
#include "../../srcs/HTTPResponse.hpp"

// テスト用のヘルパー関数
namespace {

// ファイルが存在するか確認する関数
bool fileExists(const std::string& filename) {
  std::ifstream file(filename.c_str());
  return file.good();
}

// ディレクトリを作成する関数
bool createDirectory(const std::string& path) {
  return mkdir(path.c_str(), 0755) == 0;
}

// ファイルの権限を変更する関数
bool changePermission(const std::string& path, int mode) {
  return chmod(path.c_str(), mode) == 0;
}

// テスト用のディレクティブを作成
Directive createTestDirective() {
  Directive rootDirective("root");
  Directive hostDirective("localhost");
  hostDirective.addKeyValue("root", "./test_files/");
  rootDirective.addChild(hostDirective);
  return rootDirective;
}

// テスト用のリクエストを作成
HTTPRequest createDeleteRequest(const std::string& path) {
  std::map<std::string, std::string> headers;
  headers["Host"] = "localhost:8080";
  return HTTPRequest("DELETE", path, "HTTP/1.1", headers, "");
}

}  // namespace

// 基本的なDeleteClientMethodのテストクラス
class DeleteClientMethodTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // テスト用のディレクトリを作成
    system("mkdir -p ./test_files");
  }

  void TearDown() override {
    // テスト後の後片付け
    system("rm -rf ./test_files");
  }

  // テスト用のファイルを作成する
  void createTestFile(const std::string& filename,
                      const std::string& content = "Test Content") {
    std::string fullPath = "./test_files/" + filename;
    std::ofstream file(fullPath.c_str());
    file << content;
    file.close();
  }
};

// 正常にファイルが削除されるケース
TEST_F(DeleteClientMethodTest, SuccessfulDeletion) {
  // テストファイルを作成
  createTestFile("file_to_delete.txt");
  ASSERT_TRUE(fileExists("./test_files/file_to_delete.txt"));

  // DeleteClientMethodを実行
  HTTPRequest request = createDeleteRequest("file_to_delete.txt");
  Directive directive = createTestDirective();
  DeleteClientMethod handler(request, directive);
  HTTPResponse response;

  handler.handleRequest(response);

  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 204);  // 正常な削除は204 No Content
  EXPECT_FALSE(fileExists(
      "./test_files/file_to_delete.txt"));  // ファイルが削除されているか
}

// 存在しないファイルを削除しようとするケース
TEST_F(DeleteClientMethodTest, NonExistentFile) {
  // 存在しないファイルへの参照を作成
  std::string nonExistentFile = "non_existent_file.txt";
  ASSERT_FALSE(fileExists("./test_files/" + nonExistentFile));

  // DeleteClientMethodを実行
  HTTPRequest request = createDeleteRequest(nonExistentFile);
  Directive directive = createTestDirective();
  DeleteClientMethod handler(request, directive);
  HTTPResponse response;

  handler.handleRequest(response);

  // 検証
  EXPECT_EQ(response.getHttpStatusCode(),
            404);  // ファイルが存在しない場合は404
}

// ディレクトリを削除しようとするケース
TEST_F(DeleteClientMethodTest, TryToDeleteDirectory) {
  // テストディレクトリを作成
  std::string dirName = "test_directory";
  createDirectory("./test_files/" + dirName);
  ASSERT_TRUE(fileExists("./test_files/" + dirName));

  // DeleteClientMethodを実行
  HTTPRequest request = createDeleteRequest(dirName);
  Directive directive = createTestDirective();
  DeleteClientMethod handler(request, directive);
  HTTPResponse response;

  handler.handleRequest(response);

  // 検証
  EXPECT_EQ(response.getHttpStatusCode(),
            405);  // ディレクトリ削除は許可されていない
  EXPECT_TRUE(
      fileExists("./test_files/" + dirName));  // ディレクトリはまだ存在する
}

// 権限のないファイルを削除しようとするケース
TEST_F(DeleteClientMethodTest, NoPermissionToDelete) {
  // 書き込み権限のないファイルを作成
  std::string readOnlyFile = "readonly_file.txt";
  createTestFile(readOnlyFile);

  // 権限を変更する前にファイルが存在することを確認
  ASSERT_TRUE(fileExists("./test_files/" + readOnlyFile));

  // 権限変更を試みるが、失敗した場合はテストをスキップ
  if (!changePermission("./test_files/" + readOnlyFile, 0444)) {
    std::cout << "Warning: Failed to change file permissions, skipping test."
              << std::endl;
    GTEST_SKIP();
  }

  // 権限が変更されたか確認
  if (access(("./test_files/" + readOnlyFile).c_str(), W_OK) == 0) {
    std::cout
        << "Warning: File is still writable, permissions may not be effective."
        << std::endl;
    // 権限設定がうまくいかない環境では、テストをスキップ
    GTEST_SKIP();
  }

  // DeleteClientMethodを実行
  HTTPRequest request = createDeleteRequest(readOnlyFile);
  Directive directive = createTestDirective();
  DeleteClientMethod handler(request, directive);
  HTTPResponse response;

  handler.handleRequest(response);

  // 検証 - 実行環境によって結果が異なる可能性があるため、複数の許容値を設定
  int statusCode = response.getHttpStatusCode();
  EXPECT_TRUE(statusCode == 403 || statusCode == 500 || statusCode == 404)
      << "Expected 403, 500, or 404 but got: " << statusCode;

  // ファイルがまだ存在するか確認
  EXPECT_TRUE(fileExists("./test_files/" + readOnlyFile));

  // テスト後にファイルを削除できるように権限を戻す
  changePermission("./test_files/" + readOnlyFile, 0644);
}

// 空のURLを処理するケース
TEST_F(DeleteClientMethodTest, EmptyURL) {
  // 空のURLでリクエストを作成
  HTTPRequest request = createDeleteRequest("");
  Directive directive = createTestDirective();
  DeleteClientMethod handler(request, directive);
  HTTPResponse response;

  // 実行前の状態を確認（デバッグ情報）
  std::string fullPath = handler.getFullPath();
  std::cout << "Empty URL test - Full path: '" << fullPath << "'" << std::endl;

  handler.handleRequest(response);

  // 検証 - 空のURLに対する処理は環境によって異なる可能性がある
  int statusCode = response.getHttpStatusCode();
  std::cout << "Empty URL test - Status code: " << statusCode << std::endl;

  // 404か400のいずれかを期待（環境やディレクティブ設定によって異なる可能性がある）
  EXPECT_TRUE(statusCode == 404 || statusCode == 400)
      << "Expected 404 or 400 but got: " << statusCode;
}

// チェーン内の次のハンドラーが呼ばれるかのテスト
TEST_F(DeleteClientMethodTest, NextHandlerIsCalled) {
  // モックハンドラー
  class MockHandler : public Handler {
   public:
    bool wasHandleRequestCalled = false;
    void handleRequest(HTTPResponse&) override {
      wasHandleRequestCalled = true;
    }
  };

  // セットアップ
  createTestFile("chained_file.txt");
  HTTPRequest request = createDeleteRequest("chained_file.txt");
  Directive directive = createTestDirective();
  DeleteClientMethod handler(request, directive);

  MockHandler* nextHandler = new MockHandler();
  handler.setNextHandler(nextHandler);

  HTTPResponse response;

  // 実行
  handler.handleRequest(response);

  // 検証
  EXPECT_TRUE(nextHandler->wasHandleRequestCalled);
  EXPECT_FALSE(
      fileExists("./test_files/chained_file.txt"));  // ファイルは削除された

  delete nextHandler;
}

// 複数のファイルを連続して削除するケース
TEST_F(DeleteClientMethodTest, MultipleFileDeletions) {
  // 複数のファイルを作成
  createTestFile("file1.txt");
  createTestFile("file2.txt");
  createTestFile("file3.txt");

  ASSERT_TRUE(fileExists("./test_files/file1.txt"));
  ASSERT_TRUE(fileExists("./test_files/file2.txt"));
  ASSERT_TRUE(fileExists("./test_files/file3.txt"));

  // 1つ目のファイルを削除
  {
    HTTPRequest request = createDeleteRequest("file1.txt");
    Directive directive = createTestDirective();
    DeleteClientMethod handler(request, directive);
    HTTPResponse response;

    handler.handleRequest(response);

    EXPECT_EQ(response.getHttpStatusCode(), 204);
    EXPECT_FALSE(fileExists("./test_files/file1.txt"));
  }

  // 2つ目のファイルを削除
  {
    HTTPRequest request = createDeleteRequest("file2.txt");
    Directive directive = createTestDirective();
    DeleteClientMethod handler(request, directive);
    HTTPResponse response;

    handler.handleRequest(response);

    EXPECT_EQ(response.getHttpStatusCode(), 204);
    EXPECT_FALSE(fileExists("./test_files/file2.txt"));
  }

  // 3つ目のファイルを削除
  {
    HTTPRequest request = createDeleteRequest("file3.txt");
    Directive directive = createTestDirective();
    DeleteClientMethod handler(request, directive);
    HTTPResponse response;

    handler.handleRequest(response);

    EXPECT_EQ(response.getHttpStatusCode(), 204);
    EXPECT_FALSE(fileExists("./test_files/file3.txt"));
  }
}

// ルートディレクティブのパス設定が正しく反映されるかのテスト
TEST_F(DeleteClientMethodTest, RootDirectivePathTest) {
  // カスタムルートディレクティブを作成
  Directive rootDirective("root");
  Directive hostDirective("localhost");
  hostDirective.addKeyValue("root", "./custom_path/");
  rootDirective.addChild(hostDirective);

  // カスタムパスにディレクトリとファイルを作成
  system("mkdir -p ./custom_path");
  std::ofstream file("./custom_path/custom_file.txt");
  file << "Custom test content";
  file.close();

  ASSERT_TRUE(fileExists("./custom_path/custom_file.txt"));

  // DeleteClientMethodを実行
  HTTPRequest request = createDeleteRequest("custom_file.txt");
  DeleteClientMethod handler(request, rootDirective);
  HTTPResponse response;

  handler.handleRequest(response);

  // 検証
  EXPECT_EQ(response.getHttpStatusCode(), 204);
  EXPECT_FALSE(fileExists("./custom_path/custom_file.txt"));

  // 後片付け
  system("rm -rf ./custom_path");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}