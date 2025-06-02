#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "../../srcs/Directive.hpp"
#include "../../srcs/GenerateHTTPResponse.hpp"
#include "../../srcs/HTTPRequest.hpp"
#include "../../srcs/HTTPResponse.hpp"
#include "../../srcs/StatusCodes.hpp"

// テスト用のモックハンドラークラス
class MockHandler : public Handler {
 public:
  bool handleRequestCalled = false;
  void handleRequest(HTTPResponse& httpResponse) override {
    handleRequestCalled = true;
  }
};

// テスト用のファイル作成ヘルパー関数
void createTestFile(const std::string& path, const std::string& content) {
  std::ofstream file(path.c_str());
  file << content;
  file.close();
}

class GenerateHTTPResponseTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // テスト用のファイルを作成
    createTestFile("./test_error_page.html",
                   "<html><body>Custom Error Page</body></html>");
    createTestFile("./html/test_root_error_page.html",
                   "<html><body>Root Error Page</body></html>");
  }

  void TearDown() override {
    // テストファイルの削除
    std::remove("./test_error_page.html");
    std::remove("./html/test_root_error_page.html");
  }

  // テスト用のリクエスト作成
  HTTPRequest createTestRequest(const std::string& version = "HTTP/1.1",
                                const std::string& host = "localhost") {
    std::map<std::string, std::string> headers;
    headers["Host"] = host;
    return HTTPRequest("GET", "/", version, headers, "");
  }

  // テスト用のディレクティブ作成
  Directive createRootDirective(const std::string& host,
                                const std::string& errorPath,
                                const std::string& rootPath) {
    Directive rootDirective("parent");
    Directive hostDirective(host);
    hostDirective.addKeyValue("404", errorPath);
    hostDirective.addKeyValue("root", rootPath);
    rootDirective.addChild(hostDirective);
    return rootDirective;
  }
};

// コンストラクタのテスト
TEST_F(GenerateHTTPResponseTest, ConstructorTest) {
  HTTPRequest request = createTestRequest();
  Directive rootDirective;

  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);
  // コンストラクタが例外を投げなければ成功
  SUCCEED();
}

// HTTPステータスラインの生成テスト
TEST_F(GenerateHTTPResponseTest, GenerateHttpStatusLineTest) {
  HTTPRequest request = createTestRequest();
  Directive rootDirective;
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  std::string statusLine = response.getHttpStatusLine();
  EXPECT_TRUE(statusLine.find("HTTP/1.1 404") != std::string::npos);
  EXPECT_TRUE(statusLine.find("Not Found") != std::string::npos);
}

// HTTPレスポンスヘッダーの生成テスト
TEST_F(GenerateHTTPResponseTest, GenerateHttpResponseHeaderTest) {
  HTTPRequest request = createTestRequest();
  Directive rootDirective;
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  std::string header = response.getHttpResponseHeader();
  EXPECT_TRUE(header.find("Server: webserv") != std::string::npos);
  EXPECT_TRUE(header.find("Content-Type: text/html") != std::string::npos);
  EXPECT_TRUE(header.find("Content-Length:") != std::string::npos);
  EXPECT_TRUE(header.find("Connection: close") != std::string::npos);
}

// デフォルトエラーページを使用するケースのテスト
TEST_F(GenerateHTTPResponseTest, DefaultErrorPageTest) {
  HTTPRequest request = createTestRequest();
  Directive rootDirective;
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(999);  // 存在しないステータスコード
  GenerateHTTPResponse.handleRequest(response);

  // デフォルトのエラーページが使用されるはず
  std::string body = response.getHttpResponseBody();
  // ./html/defaultErrorPage.htmlを読み込む
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();
  EXPECT_EQ(body, defaultErrorPage);
}

// 次のハンドラーにリクエストが渡されるテスト
TEST_F(GenerateHTTPResponseTest, NextHandlerTest) {
  HTTPRequest request = createTestRequest();
  Directive rootDirective;
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  // モックハンドラーを設定
  MockHandler* mockHandler = new MockHandler();
  GenerateHTTPResponse.setNextHandler(mockHandler);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  // 次のハンドラーが呼び出されたか確認
  EXPECT_TRUE(mockHandler->handleRequestCalled);

  delete mockHandler;
}

// error_pageディレクティブがない場合のテスト
TEST_F(GenerateHTTPResponseTest, NoErrorPageDirectiveTest) {
  // error_pageディレクティブを持たないDirectiveを作成
  Directive rootDirective("root");
  Directive hostDirective("localhost");
  rootDirective.addChild(hostDirective);

  HTTPRequest request = createTestRequest();
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  // デフォルトのエラーページが使用されるはず
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();
  EXPECT_EQ(response.getHttpResponseBody(), defaultErrorPage);
}

// 特定のステータスコードに対応するエラーページが定義されていない場合のテスト
TEST_F(GenerateHTTPResponseTest, NoMatchingStatusCodeTest) {
  // 404以外のステータスコード（例：500）に対応するエラーページのみ定義
  Directive rootDirective("root");
  Directive hostDirective("localhost");
  Directive errorPageDirective("error_page");
  errorPageDirective.addKeyValue("500", "/500_error.html");
  hostDirective.addChild(errorPageDirective);
  hostDirective.addKeyValue("root", ".");
  rootDirective.addChild(hostDirective);

  HTTPRequest request = createTestRequest();
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);  // 定義されていないステータスコード
  GenerateHTTPResponse.handleRequest(response);

  // デフォルトのエラーページが使用されるはず
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();
  EXPECT_EQ(response.getHttpResponseBody(), defaultErrorPage);
}

// hostディレクティブがない場合のテスト
TEST_F(GenerateHTTPResponseTest, NoHostDirectiveTest) {
  // 存在しないホスト名を使用
  HTTPRequest request = createTestRequest("HTTP/1.1", "unknown-host.com");
  Directive rootDirective("root");
  // localhostだけ定義して、unknown-host.comは定義しない
  Directive hostDirective("localhost");
  rootDirective.addChild(hostDirective);

  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  // デフォルトのエラーページが使用されるはず
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();
  EXPECT_EQ(response.getHttpResponseBody(), defaultErrorPage);
}

// rootが設定されていない場合のテスト
TEST_F(GenerateHTTPResponseTest, NoRootValueTest) {
  // rootキーがない設定
  Directive rootDirective("root");
  Directive hostDirective("localhost");
  Directive errorPageDirective("error_page");
  errorPageDirective.addKeyValue("404", "/test_error_page.html");
  hostDirective.addChild(errorPageDirective);
  // rootは設定しない
  rootDirective.addChild(hostDirective);

  HTTPRequest request = createTestRequest();
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  // パスが正しくなくても、rootが空なので相対パスで見つけられるか試みる
  // デフォルトのエラーページが使用されるはず
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();
  EXPECT_EQ(response.getHttpResponseBody(), defaultErrorPage);
}

// エラーページのパスが間違っている場合のテスト
TEST_F(GenerateHTTPResponseTest, InvalidErrorPagePathTest) {
  Directive rootDirective =
      createRootDirective("localhost", "/non_existent_page.html", ".");

  HTTPRequest request = createTestRequest();
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  HTTPResponse response;
  response.setHttpStatusCode(404);
  GenerateHTTPResponse.handleRequest(response);

  // 存在しないパスなのでデフォルトのエラーページが使用されるはず
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();
  EXPECT_EQ(response.getHttpResponseBody(), defaultErrorPage);
}

// 複数のホストと異なるステータスコードのテスト
TEST_F(GenerateHTTPResponseTest, MultipleHostsAndStatusCodesTest) {
  // 異なるホスト用のエラーページを作成
  createTestFile("./host1_404.html", "<html>Host1 404 Page</html>");
  createTestFile("./host2_404.html", "<html>Host2 404 Page</html>");
  createTestFile("./host1_500.html", "<html>Host1 500 Page</html>");

  // 複数のホストとステータスコード設定を持つDirectiveを作成
  Directive rootDirective("root");

  // ホスト1の設定
  Directive host1Directive("example.com");
  host1Directive.addKeyValue("root", ".");
  Directive errorPage1Directive("error_page");
  errorPage1Directive.addKeyValue("404", "/host1_404.html");
  errorPage1Directive.addKeyValue("500", "/host1_500.html");
  host1Directive.addChild(errorPage1Directive);
  rootDirective.addChild(host1Directive);

  // ホスト2の設定
  Directive host2Directive("another.com");
  host2Directive.addKeyValue("root", ".");
  Directive errorPage2Directive("error_page");
  errorPage2Directive.addKeyValue("404", "/host2_404.html");
  host2Directive.addChild(errorPage2Directive);
  rootDirective.addChild(host2Directive);

  // ホスト1の404エラーページをテスト
  {
    HTTPRequest request1 = createTestRequest("HTTP/1.1", "example.com");
    GenerateHTTPResponse GenerateHTTPResponse1(rootDirective, request1);
    HTTPResponse response1;
    response1.setHttpStatusCode(404);
    GenerateHTTPResponse1.handleRequest(response1);
    EXPECT_EQ(response1.getHttpResponseBody(), "<html>Host1 404 Page</html>");
  }

  // ホスト2の404エラーページをテスト
  {
    HTTPRequest request2 = createTestRequest("HTTP/1.1", "another.com");
    GenerateHTTPResponse GenerateHTTPResponse2(rootDirective, request2);
    HTTPResponse response2;
    response2.setHttpStatusCode(404);
    GenerateHTTPResponse2.handleRequest(response2);
    EXPECT_EQ(response2.getHttpResponseBody(), "<html>Host2 404 Page</html>");
  }

  // ホスト1の500エラーページをテスト
  {
    HTTPRequest request3 = createTestRequest("HTTP/1.1", "example.com");
    GenerateHTTPResponse GenerateHTTPResponse3(rootDirective, request3);
    HTTPResponse response3;
    response3.setHttpStatusCode(500);
    GenerateHTTPResponse3.handleRequest(response3);
    EXPECT_EQ(response3.getHttpResponseBody(), "<html>Host1 500 Page</html>");
  }

  // テスト後にファイル削除
  std::remove("./host1_404.html");
  std::remove("./host2_404.html");
  std::remove("./host1_500.html");
}

// GenerateHTTPResponse::getPathForHttpResponseBodyの単体テスト
class GetPathForHTTPResponseBodyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // テスト用のディレクトリと必要なファイルを作成
    system("mkdir -p ./test_root/dir/");
    createTestFile("./test_root/dir/index.html", "<html>Index Page</html>");
    createTestFile("./test_root/custom_error.html",
                   "<html>Custom Error</html>");
    createTestFile("./test_root/file.html", "<html>Regular File</html>");
  }

  void TearDown() override {
    // テスト用のファイルとディレクトリを削除
    system("rm -rf ./test_root");
  }

  // publicメソッドを通して間接的にgetPathForHttpResponseBodyをテストする
  std::string testPathForHttpResponseBody(Directive& rootDirective,
                                          HTTPRequest& request,
                                          int statusCode) {
    // GenerateHTTPResponseとHTTPResponseのインスタンスを作成
    GenerateHTTPResponse generator(rootDirective, request);
    HTTPResponse response;

    // ステータスコードを設定
    response.setHttpStatusCode(statusCode);

    // handleRequestを呼び出す前のbodyを退避
    std::string originalBody = "<!-- original body -->";
    response.setHttpResponseBody(originalBody);

    // handleRequestを呼び出す
    generator.handleRequest(response);

    // レスポンスボディを取得
    std::string resultBody = response.getHttpResponseBody();

    // ファイルパスを抽出するためにファイルの内容を読み込む
    // 実際のテストでは、生成されるレスポンスボディの中身をチェックする
    return resultBody;
  }
};

// エラーステータスコード(404)とカスタムエラーページが設定されている場合
TEST_F(GetPathForHTTPResponseBodyTest, ErrorStatusWithCustomErrorPage) {
  // 設定を作成
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  hostDirective.addKeyValue("root", "./test_root");
  Directive errorPageDirective("error_page");
  errorPageDirective.addKeyValue("404", "/custom_error.html");
  hostDirective.addChild(errorPageDirective);
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("GET", "/some/path", "HTTP/1.1", headers, "");

  // メソッド呼び出し
  std::string body = testPathForHttpResponseBody(rootDirective, request, 404);

  // 期待される結果: カスタムエラーページの内容
  EXPECT_EQ(body, "<html>Custom Error</html>");
}

// エラーステータスコード(404)だがカスタムエラーページが設定されていない場合
TEST_F(GetPathForHTTPResponseBodyTest, ErrorStatusWithoutCustomErrorPage) {
  // 設定を作成
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  hostDirective.addKeyValue("root", "./test_root");
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("GET", "/some/path", "HTTP/1.1", headers, "");

  // メソッド呼び出し
  std::string body = testPathForHttpResponseBody(rootDirective, request, 404);

  // 期待される結果: デフォルトのエラーページの内容
  // デフォルトのエラーページの内容を読み込む
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();

  EXPECT_EQ(body, defaultErrorPage);
}

// 成功ステータスコード(200)でURLがディレクトリの場合
TEST_F(GetPathForHTTPResponseBodyTest, SuccessStatusWithDirectoryURL) {
  // 設定を作成
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  hostDirective.addKeyValue("root", "./test_root");
  rootDirective.addChild(hostDirective);

  // リクエスト作成 (URLの末尾にスラッシュがある = ディレクトリ)
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("GET", "/dir/", "HTTP/1.1", headers, "");

  // メソッド呼び出し
  std::string body = testPathForHttpResponseBody(rootDirective, request, 200);

  // 期待される結果: ディレクトリのインデックスファイルの内容
  EXPECT_EQ(body, "<html>Index Page</html>");
}

// 成功ステータスコード(200)でURLがファイルの場合
TEST_F(GetPathForHTTPResponseBodyTest, SuccessStatusWithFileURL) {
  // 設定を作成
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  hostDirective.addKeyValue("root", "./test_root");
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("GET", "/file.html", "HTTP/1.1", headers, "");

  // メソッド呼び出し
  std::string body = testPathForHttpResponseBody(rootDirective, request, 200);

  // 期待される結果: ファイルの内容
  EXPECT_EQ(body, "<html>Regular File</html>");
}

// ホストディレクティブが見つからない場合
TEST_F(GetPathForHTTPResponseBodyTest, HostDirectiveNotFound) {
  // 設定を作成 (異なるホスト名)
  Directive rootDirective("root");
  Directive hostDirective("other.com");
  hostDirective.addKeyValue("root", "./test_root");
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";  // 設定には存在しないホスト
  HTTPRequest request("GET", "/file.html", "HTTP/1.1", headers, "");

  // メソッド呼び出し (エラーステータスコード)
  std::string body = testPathForHttpResponseBody(rootDirective, request, 404);

  // 期待される結果: デフォルトのエラーページの内容
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();

  EXPECT_EQ(body, defaultErrorPage);
}

// rootの値がない場合
TEST_F(GetPathForHTTPResponseBodyTest, NoRootValue) {
  // 設定を作成 (rootキーなし)
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  // rootは設定しない
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("GET", "/file.html", "HTTP/1.1", headers, "");

  // メソッド呼び出し (成功ステータスコード)
  std::string body = testPathForHttpResponseBody(rootDirective, request, 200);

  // ファイルが見つからないはずなので、デフォルトのエラーページになるはず
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();

  EXPECT_EQ(body, defaultErrorPage);
}

// 別のエラーステータスコードが設定されている場合(500のみ設定されていて、404を要求)
TEST_F(GetPathForHTTPResponseBodyTest, DifferentErrorStatusCode) {
  // 設定を作成
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  hostDirective.addKeyValue("root", "./test_root");
  Directive errorPageDirective("error_page");
  errorPageDirective.addKeyValue(
      "500", "/custom_error.html");  // 500は設定されているが404はない
  hostDirective.addChild(errorPageDirective);
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("GET", "/some/path", "HTTP/1.1", headers, "");

  // メソッド呼び出し
  std::string body = testPathForHttpResponseBody(rootDirective, request, 404);

  // 期待される結果: デフォルトのエラーページの内容
  std::ifstream file(DEFAULT_ERROR_PAGE);
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string defaultErrorPage = buffer.str();

  EXPECT_EQ(body, defaultErrorPage);
}

// DELETEメソッドがコールされた場合，HTTPレスポンスボディを空にしているかテストする
TEST(GerateHTTPResponseBodyTest, DeleteMethod) {
  // テスト用のディレクティブを作成
  Directive rootDirective("root");
  Directive hostDirective("example.com");
  hostDirective.addKeyValue("root", "./test_root");
  rootDirective.addChild(hostDirective);

  // リクエスト作成
  std::map<std::string, std::string> headers;
  headers["Host"] = "example.com";
  HTTPRequest request("DELETE", "/some/path", "HTTP/1.1", headers, "");

  // GenerateHTTPResponseインスタンスを作成
  GenerateHTTPResponse GenerateHTTPResponse(rootDirective, request);

  // HTTPResponseインスタンスを作成
  HTTPResponse response;
  response.setHttpStatusCode(200);

  // handleRequestを呼び出す前のbodyを退避
  std::string originalBody = "<!-- original body -->";
  response.setHttpResponseBody(originalBody);

  // handleRequestを呼び出す
  GenerateHTTPResponse.handleRequest(response);

  // DELETEメソッドの場合，HTTPレスポンスボディは空になる
  EXPECT_EQ(response.getHttpResponseBody(), "");
}
