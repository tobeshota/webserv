#include <gtest/gtest.h>
#include <sys/stat.h>

#include <cstdlib>
#include <fstream>

#include "../../srcs/CGI.hpp"

// ハンドラーチェーンのテスト用の具象ハンドラークラス
class TestHandler : public Handler {
 private:
  bool _called;

 public:
  TestHandler() : _called(false) {}

  virtual void handleRequest(HTTPResponse& httpResponse) { _called = true; }

  bool wasCalled() const { return _called; }
};

// テスト用のHTTPRequest作成ヘルパー関数
HTTPRequest createTestRequest(const std::string& method,
                              const std::string& url) {
  std::map<std::string, std::string> headers;
  headers["Host"] = "localhost";
  headers["Content-Type"] = "text/plain";
  headers["Content-Length"] = "10";
  return HTTPRequest(method, url, "HTTP/1.1", headers, "test-body", false);
}

// テスト用のDirective作成ヘルパー関数
Directive createTestDirective() {
  Directive rootDirective("root");

  // ホスト設定
  Directive hostDirective("localhost");
  hostDirective.addKeyValue("root", "/tmp/webserv/www");
  rootDirective.addChild(hostDirective);

  // エラーページ設定
  Directive errorPageDirective("error_page");
  errorPageDirective.addKeyValue("500", "/error/500.html");
  hostDirective.addChild(errorPageDirective);

  // ディレクトリのlocationディレクティブを追加
  Directive locationDirective("location");
  locationDirective.setName("/testdir/");
  locationDirective.addKeyValue("index", "index.py");
  hostDirective.addChild(locationDirective);

  return rootDirective;
}

// テスト用のPythonスクリプトを作成
void createTestPythonScript(const std::string& path,
                            const std::string& content) {
  // ディレクトリが存在しない場合は作成
  std::string dir = path.substr(0, path.find_last_of('/'));
  std::string command = "mkdir -p " + dir;
  system(command.c_str());

  std::ofstream scriptFile(path.c_str());
  scriptFile << content;
  scriptFile.close();

  // 実行権限を付与
  chmod(path.c_str(), 0755);
}

// テスト用のシェルスクリプトを作成
void createTestShellScript(const std::string& path,
                           const std::string& content) {
  // ディレクトリが存在しない場合は作成
  std::string dir = path.substr(0, path.find_last_of('/'));
  std::string command = "mkdir -p " + dir;
  system(command.c_str());

  std::ofstream scriptFile(path.c_str());
  scriptFile << content;
  scriptFile.close();

  // 実行権限を付与
  chmod(path.c_str(), 0755);
}

// CGIテストフィクスチャ
class CGITest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // テスト環境のセットアップ
    system("mkdir -p /tmp/webserv/www");

    // テスト用のPythonスクリプトを作成
    createTestPythonScript(
        "/tmp/webserv/www/test.py",
        "#!/usr/bin/env python\n"
        "print(\"Content-Type: text/html\\n\\n\")\n"
        "print(\"<html><body>Hello from CGI</body></html>\")\n");

    createTestPythonScript(
        "/tmp/webserv/www/timeout.py",
        "#!/usr/bin/env python\n"
        "import time\n"
        "time.sleep(35)\n"  // タイムアウトテスト用
        "print(\"Content-Type: text/html\\n\\n\")\n"
        "print(\"<html><body>This should timeout</body></html>\")\n");

    createTestPythonScript("/tmp/webserv/www/error.py",
                           "#!/usr/bin/env python\n"
                           "import sys\n"
                           "sys.exit(1)\n");  // エラー終了テスト用

    // POSTデータ処理テスト用
    createTestPythonScript(
        "/tmp/webserv/www/post.py",
        "#!/usr/bin/env python\n"
        "import sys\n"
        "data = sys.stdin.read()\n"
        "print(\"Content-Type: text/html\\n\\n\")\n"
        "print(\"<html><body>Received: \" + data + \"</body></html>\")\n");

    // 環境変数表示テスト用
    createTestPythonScript("/tmp/webserv/www/env.py",
                           "#!/usr/bin/env python\n"
                           "import os\n"
                           "print(\"Content-Type: text/html\\n\\n\")\n"
                           "print(\"<html><body>\")\n"
                           "for key, value in os.environ.items():\n"
                           "    print(key + \": \" + value + \"<br>\")\n"
                           "print(\"</body></html>\")\n");

    // ディレクトリとインデックスファイルのテスト用
    system("mkdir -p /tmp/webserv/www/testdir/");
    createTestPythonScript(
        "/tmp/webserv/www/testdir/index.py",
        "#!/usr/bin/env python\n"
        "print(\"Content-Type: text/html\\n\\n\")\n"
        "print(\"<html><body>Directory Index</body></html>\")\n");

    // テスト用のシェルスクリプトを作成
    createTestShellScript(
        "/tmp/webserv/www/test.sh",
        "#!/bin/sh\n"
        "echo \"Content-Type: text/html\"\n"
        "echo \"\"\n"
        "echo \"<html><body>Hello from Shell Script</body></html>\"\n");

    createTestShellScript(
        "/tmp/webserv/www/timeout_sh.sh",
        "#!/bin/sh\n"
        "sleep 35\n"  // タイムアウトテスト用
        "echo \"Content-Type: text/html\"\n"
        "echo \"\"\n"
        "echo \"<html><body>This should timeout</body></html>\"\n");

    createTestShellScript("/tmp/webserv/www/error_sh.sh",
                          "#!/bin/sh\n"
                          "exit 1\n");  // エラー終了テスト用

    // POSTデータ処理テスト用シェルスクリプト
    createTestShellScript(
        "/tmp/webserv/www/post_sh.sh",
        "#!/bin/sh\n"
        "read POST_DATA\n"
        "echo \"Content-Type: text/html\"\n"
        "echo \"\"\n"
        "echo \"<html><body>Received: $POST_DATA</body></html>\"\n");

    // 環境変数表示テスト用シェルスクリプト
    createTestShellScript("/tmp/webserv/www/env_sh.sh",
                          "#!/bin/sh\n"
                          "echo \"Content-Type: text/html\"\n"
                          "echo \"\"\n"
                          "echo \"<html><body>\"\n"
                          "env | sort | while read line; do\n"
                          "  echo \"$line<br>\"\n"
                          "done\n"
                          "echo \"</body></html>\"\n");

    // ディレクトリとインデックスファイルのテスト用(シェルスクリプト版)
    system("mkdir -p /tmp/webserv/www/testdir_sh/");
    createTestShellScript(
        "/tmp/webserv/www/testdir_sh/index.sh",
        "#!/bin/sh\n"
        "echo \"Content-Type: text/html\"\n"
        "echo \"\"\n"
        "echo \"<html><body>Directory Index from Shell</body></html>\"\n");
  }

  virtual void TearDown() {
    // テスト環境のクリーンアップ
    system("rm -rf /tmp/webserv");
    system("rm -f /tmp/.cgi_response.html");
    system("rm -f /tmp/.cgi_post_data");
  }

  // ファイルが存在するか確認するヘルパーメソッド
  bool fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
  }

  // ファイル内容を読み込むヘルパーメソッド
  std::string readFileContents(const std::string& path) {
    std::ifstream file(path.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }
};

// Pythonスクリプト拡張子の認識テスト - 間接的に実装
TEST_F(CGITest, PythonScriptDetectionTest) {
  Directive rootDirective = createTestDirective();

  // Pythonスクリプトの場合 - 実行されるはず
  HTTPRequest pyRequest = createTestRequest("GET", "/test.py");
  CGI cgiHandler1(rootDirective, pyRequest);
  HTTPResponse response1;
  cgiHandler1.handleRequest(response1);

  // レスポンスが処理されたことを確認（ステータスコードが設定されている）
  EXPECT_EQ(200, response1.getHttpStatusCode());

  // Pythonスクリプトではない場合 - 実行されないはず
  HTTPRequest htmlRequest = createTestRequest("GET", "/index.html");
  CGI cgiHandler2(rootDirective, htmlRequest);
  HTTPResponse response2;
  cgiHandler2.handleRequest(response2);

  // レスポンスが処理されていないことを確認（ステータスコードが設定されていない）
  EXPECT_EQ(0, response2.getHttpStatusCode());
}

// スクリプトパスの解決テスト - 間接的に実装
TEST_F(CGITest, ScriptPathResolutionTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/test.py");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // スクリプトが正しく実行されたことを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // 存在しないスクリプトパスの場合
  HTTPRequest badRequest = createTestRequest("GET", "/nonexistent.py");
  CGI badCgiHandler(rootDirective, badRequest);
  HTTPResponse badResponse;
  badCgiHandler.handleRequest(badResponse);

  // スクリプトが見つからないので失敗するはず
  EXPECT_EQ(404, badResponse.getHttpStatusCode());
}

// CGI実行（成功）のテスト
TEST_F(CGITest, ExecuteCGISuccessTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/test.py");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // レスポンスのステータスコードが200であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIの出力ファイルが生成されていることを確認
  EXPECT_TRUE(fileExists(CGI_PAGE));

  // CGIの出力内容に期待する文字列が含まれていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("Hello from CGI") != std::string::npos);
}

// CGI実行（失敗）のテスト
TEST_F(CGITest, ExecuteCGIFailureTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/nonexistent.py");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // スクリプトが存在しない場合、404エラーになるはず（500から404に変更）
  EXPECT_EQ(404, response.getHttpStatusCode());
}

// エラー終了のスクリプトテスト
TEST_F(CGITest, ExecuteCGIErrorExitTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/error.py");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // エラー終了するスクリプトの場合、500エラーになるはず
  EXPECT_EQ(500, response.getHttpStatusCode());
}

// タイムアウトのテスト（注：実際の実行時間がかかります）
TEST_F(CGITest, ExecuteCGITimeoutTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/timeout.py");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // タイムアウトする場合、500エラーになるはず
  EXPECT_EQ(500, response.getHttpStatusCode());
}

// POSTリクエスト処理のテスト
TEST_F(CGITest, ExecuteCGIWithPostDataTest) {
  Directive rootDirective = createTestDirective();

  std::map<std::string, std::string> headers;
  headers["Host"] = "localhost";
  headers["Content-Type"] = "application/x-www-form-urlencoded";
  headers["Content-Length"] = "9";

  HTTPRequest request("POST", "/post.py", "HTTP/1.1", headers, "name=test",
                      false);

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // レスポンスのステータスコードが200であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIの出力ファイルが生成されていることを確認
  EXPECT_TRUE(fileExists(CGI_PAGE));

  // CGIの出力内容にPOSTデータが反映されていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("Received: name=test") != std::string::npos);
}

// 環境変数テスト
TEST_F(CGITest, EnvironmentVariablesTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/env.py?param=value");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // レスポンスのステータスコードが200であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIの出力ファイルが生成されていることを確認
  EXPECT_TRUE(fileExists(CGI_PAGE));

  // 環境変数が正しく設定されていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("REQUEST_METHOD: GET") != std::string::npos);
  EXPECT_TRUE(content.find("QUERY_STRING: param=value") != std::string::npos);
  EXPECT_TRUE(content.find("SERVER_NAME: localhost") != std::string::npos);
}

// ハンドラーチェーンのテスト
TEST_F(CGITest, HandlerChainTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/test.py");

  CGI cgiHandler(rootDirective, request);
  TestHandler testHandler;  // Google Mockを使わない具象クラス
  cgiHandler.setNextHandler(&testHandler);

  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // 次のハンドラーが呼び出されたことを確認
  EXPECT_TRUE(testHandler.wasCalled());
}

// 非Pythonスクリプトの場合のハンドラーチェーン
TEST_F(CGITest, NonPythonScriptTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/index.html");

  CGI cgiHandler(rootDirective, request);
  TestHandler testHandler;
  cgiHandler.setNextHandler(&testHandler);

  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // CGI処理は実行されないが、次のハンドラーは呼び出されるはず
  EXPECT_TRUE(testHandler.wasCalled());

  // CGI処理は実行されないので、ステータスコードは変更されないはず
  EXPECT_EQ(0, response.getHttpStatusCode());
}

// ディレクトリパス解決テスト
TEST_F(CGITest, DirectoryPathResolutionTest) {
  Directive rootDirective = createTestDirective();

  // ディレクトリへのリクエスト（末尾にスラッシュあり）
  HTTPRequest dirRequest = createTestRequest("GET", "/testdir/");
  CGI cgiHandler(rootDirective, dirRequest);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // インデックスファイルが正しく解決されたことを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIが実行された場合、出力ファイルが生成されているはず
  if (fileExists(CGI_PAGE)) {
    // CGIの出力内容に期待する文字列が含まれていることを確認
    std::string content = readFileContents(CGI_PAGE);
    EXPECT_TRUE(content.find("Directory Index") != std::string::npos);
  } else {
    // ファイルが生成されていない場合は、HTTPResponseボディがディレクトリリストである
    std::string content = response.getHttpResponseBody();
    EXPECT_TRUE(content.find("Directory Index: /testdir/") !=
                std::string::npos);
  }
}

// パス解決のエッジケースをテスト
TEST_F(CGITest, PathResolutionEdgeCasesTest) {
  Directive rootDirective = createTestDirective();

  // 異なるホスト名でのリクエスト
  std::map<std::string, std::string> headers;
  headers["Host"] = "unknown-host";
  HTTPRequest unknownHostRequest("GET", "/test.py", "HTTP/1.1", headers, "",
                                 false);

  CGI cgiHandler(rootDirective, unknownHostRequest);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // ホストが不明な場合はroot値が取得できず、スクリプトが見つからないので404になるはず
  EXPECT_EQ(404, response.getHttpStatusCode());
}

// メソッドのパラメータ解析テスト
TEST_F(CGITest, QueryParameterTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/env.py?name=value&test=123");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // レスポンスが正常であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // クエリ文字列が正しく環境変数に設定されていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("QUERY_STRING: name=value&test=123") !=
              std::string::npos);
}

TEST_F(CGITest, DirectoryListingGenerationTest) {
  // テスト用ディレクトリを作成（インデックスファイルなし）
  system("mkdir -p /tmp/webserv/www/noindex/");
  system("touch /tmp/webserv/www/noindex/test1.txt");
  system("touch /tmp/webserv/www/noindex/test2.html");

  // Create test directive structure that includes the noindex location
  Directive rootDirective = createTestDirective();

  // Add a location for the noindex directory directly when creating the
  // directive structure This assumes you can modify createTestDirective() or
  // create a new helper method

  // For example, something like:
  Directive hostDirective("server");
  hostDirective.setName("localhost");

  Directive noindexDirective("location");
  noindexDirective.setName("/noindex/");
  hostDirective.addChild(noindexDirective);

  rootDirective.addChild(hostDirective);

  // Or alternatively, if you need to keep using findDirective(), check your
  // class definition and consider adding a non-const version of findDirective()

  HTTPRequest request = createTestRequest("GET", "/noindex/");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // ディレクトリリストが生成され、200ステータスコードが設定されたことを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // ディレクトリリストがHTTPResponseボディに設定されたことを確認
  std::string content = response.getHttpResponseBody();
  EXPECT_TRUE(content.find("Directory Index: /noindex/") != std::string::npos);
  EXPECT_TRUE(content.find("test1.txt") != std::string::npos);
  EXPECT_TRUE(content.find("test2.html") != std::string::npos);

  // Clean up test files
  system("rm -rf /tmp/webserv/www/noindex/");
}

// シェルスクリプト拡張子の認識テスト
TEST_F(CGITest, ShellScriptDetectionTest) {
  Directive rootDirective = createTestDirective();

  // シェルスクリプトの場合 - 実行されるはず
  HTTPRequest shRequest = createTestRequest("GET", "/test.sh");
  CGI cgiHandler1(rootDirective, shRequest);
  HTTPResponse response1;
  cgiHandler1.handleRequest(response1);

  // レスポンスが処理されたことを確認（ステータスコードが設定されている）
  EXPECT_EQ(200, response1.getHttpStatusCode());

  // シェルスクリプトではない場合 - 実行されないはず
  HTTPRequest htmlRequest = createTestRequest("GET", "/index.html");
  CGI cgiHandler2(rootDirective, htmlRequest);
  HTTPResponse response2;
  cgiHandler2.handleRequest(response2);

  // レスポンスが処理されていないことを確認（ステータスコードが設定されていない）
  EXPECT_EQ(0, response2.getHttpStatusCode());
}

// シェルスクリプト実行（成功）のテスト
TEST_F(CGITest, ExecuteShellCGISuccessTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/test.sh");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // レスポンスのステータスコードが200であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIの出力ファイルが生成されていることを確認
  EXPECT_TRUE(fileExists(CGI_PAGE));

  // CGIの出力内容に期待する文字列が含まれていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("Hello from Shell Script") != std::string::npos);
}

// エラー終了のシェルスクリプトテスト
TEST_F(CGITest, ExecuteShellCGIErrorExitTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/error_sh.sh");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // エラー終了するスクリプトの場合、500エラーになるはず
  EXPECT_EQ(500, response.getHttpStatusCode());
}

// タイムアウトのシェルスクリプトテスト（注：実際の実行時間がかかります）
TEST_F(CGITest, ExecuteShellCGITimeoutTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/timeout_sh.sh");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // タイムアウトする場合、500エラーになるはず
  EXPECT_EQ(500, response.getHttpStatusCode());
}

// POSTリクエスト処理のシェルスクリプトテスト
TEST_F(CGITest, ExecuteShellCGIWithPostDataTest) {
  Directive rootDirective = createTestDirective();

  std::map<std::string, std::string> headers;
  headers["Host"] = "localhost";
  headers["Content-Type"] = "application/x-www-form-urlencoded";
  headers["Content-Length"] = "9";

  HTTPRequest request("POST", "/post_sh.sh", "HTTP/1.1", headers, "name=test",
                      false);

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // レスポンスのステータスコードが200であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIの出力ファイルが生成されていることを確認
  EXPECT_TRUE(fileExists(CGI_PAGE));

  // CGIの出力内容にPOSTデータが反映されていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("Received: name=test") != std::string::npos);
}

// 環境変数シェルスクリプトテスト
TEST_F(CGITest, ShellEnvironmentVariablesTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/env_sh.sh?param=value");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;

  cgiHandler.handleRequest(response);

  // レスポンスのステータスコードが200であることを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIの出力ファイルが生成されていることを確認
  EXPECT_TRUE(fileExists(CGI_PAGE));

  // 環境変数が正しく設定されていることを確認
  std::string content = readFileContents(CGI_PAGE);
  EXPECT_TRUE(content.find("REQUEST_METHOD=GET") != std::string::npos);
  EXPECT_TRUE(content.find("QUERY_STRING=param=value") != std::string::npos);
  EXPECT_TRUE(content.find("SERVER_NAME=localhost") != std::string::npos);
}

// シェルスクリプトを使ったディレクトリインデックスのテスト
TEST_F(CGITest, ShellScriptDirectoryIndexTest) {
  // テスト用のディレクトリ設定を追加
  Directive rootDirective = createTestDirective();

  // シェルスクリプトをインデックスとして使用するlocationディレクティブを追加
  Directive hostDirective = *(rootDirective.findDirective("localhost"));
  Directive shellLocationDirective("location");
  shellLocationDirective.setName("/testdir_sh/");
  shellLocationDirective.addKeyValue("index", "index.sh");
  hostDirective.addChild(shellLocationDirective);

  // ディレクトリへのリクエスト
  HTTPRequest dirRequest = createTestRequest("GET", "/testdir_sh/");
  CGI cgiHandler(rootDirective, dirRequest);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // インデックスファイルが正しく解決されたことを確認
  EXPECT_EQ(200, response.getHttpStatusCode());

  // CGIが実行された場合、出力ファイルが生成されているはず
  if (fileExists(CGI_PAGE)) {
    // CGIの出力内容に期待する文字列が含まれていることを確認
    std::string content = readFileContents(CGI_PAGE);
    EXPECT_TRUE(content.find("Directory Index from Shell") !=
                std::string::npos);
  }
}

// 追加：存在しないディレクトリのテスト
TEST_F(CGITest, NonExistentDirectoryTest) {
  Directive rootDirective = createTestDirective();
  HTTPRequest request = createTestRequest("GET", "/nonexistent-dir/");

  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // 存在しないディレクトリの場合、404エラーになるはず
  EXPECT_EQ(404, response.getHttpStatusCode());
}

// 追加：存在しないインデックスファイルのテスト
TEST_F(CGITest, NonExistentIndexTest) {
  // テスト用ディレクトリを作成（インデックスファイルなし）
  system("mkdir -p /tmp/webserv/www/empty-dir/");

  // indexがmissing.pyを指すlocationディレクティブを追加
  Directive rootDirective = createTestDirective();
  Directive hostDirective = *(rootDirective.findDirective("localhost"));
  Directive emptyDirLocation("location");
  emptyDirLocation.setName("/empty-dir/");
  emptyDirLocation.addKeyValue("index", "missing.py");
  hostDirective.addChild(emptyDirLocation);

  HTTPRequest request = createTestRequest("GET", "/empty-dir/");
  CGI cgiHandler(rootDirective, request);
  HTTPResponse response;
  cgiHandler.handleRequest(response);

  // インデックスファイルが存在しない場合、404エラーになるはず
  EXPECT_EQ(404, response.getHttpStatusCode());

  // クリーンアップ
  system("rm -rf /tmp/webserv/www/empty-dir/");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
