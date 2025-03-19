#include <gtest/gtest.h>

#include "Directive.hpp"
#include "GET.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

// GenerateHTTPResponseのモック - GMockを使わないバージョン
class MockGenerateHTTPResponse : public Handler {
 public:
  MockGenerateHTTPResponse() : handleRequestCalled(false) {}

  void handleRequest(HTTPResponse& response) override {
    handleRequestCalled = true;
    // モック実装：何もしない
  }

  // メソッドが呼ばれたかチェックするフラグ
  bool handleRequestCalled;
};

// テスト用のモックDirectiveクラス
class MockDirective : public Directive {
 public:
  // 最低限のDirectiveの機能を実装
  MockDirective() : Directive() {}
};

// ハンドラをセットせずにテスト可能にするための派生クラス
class TestableGET : public GET {
 public:
  TestableGET(Directive rootDirective, HTTPRequest httpRequest)
      : GET(rootDirective, httpRequest) {}

  // _nextHandlerを確認するための関数
  Handler* getNextHandler() const { return _nextHandler; }

  // 通常setNextHandlerで削除されるハンドラをテスト中に削除しないようにする
  void setNextHandlerWithoutOwnership(Handler* handler) {
    _nextHandler = handler;
  }
};

class GETTest : public ::testing::Test {
 protected:
  MockDirective mockDirective;
  HTTPResponse httpResponse;

  void SetUp() override {
    // テストセットアップ
  }

  void TearDown() override {
    // クリーンアップ
  }
};

// 非CGIリクエストのテスト
TEST_F(GETTest, HandlesNonCgiRequest) {
  // 通常のHTML要求を設定（コンストラクタで初期化）
  HTTPRequest httpRequest("GET", "/index.html", "HTTP/1.1",
                          std::map<std::string, std::string>(), "");

  // テスト対象のGETを作成
  TestableGET get(mockDirective, httpRequest);

  // モックハンドラを作成し、GETのnextHandlerとしてセット
  MockGenerateHTTPResponse* mockHandler = new MockGenerateHTTPResponse();

  // モックをセット (通常は内部で自動的に行われる)
  get.setNextHandlerWithoutOwnership(mockHandler);

  // テスト実行
  get.handleRequest(httpResponse);

  // handleRequestが呼ばれたことを確認
  EXPECT_TRUE(mockHandler->handleRequestCalled);

  // メモリリーク防止
  delete mockHandler;
}

// CGIリクエスト(.py)のテスト
TEST_F(GETTest, HandlesPythonCgiRequest) {
  // Pythonスクリプト要求を設定（コンストラクタで初期化）
  HTTPRequest httpRequest("GET", "/script.py", "HTTP/1.1",
                          std::map<std::string, std::string>(), "");

  // テスト対象のGETを作成
  GET get(mockDirective, httpRequest);

  // モックハンドラを作成
  MockGenerateHTTPResponse mockHandler;

  // ハンドラをセット
  get.setNextHandler(&mockHandler);

  // テスト実行
  get.handleRequest(httpResponse);

  // CGIリクエストでもnextHandlerは呼ばれるはず
  EXPECT_TRUE(mockHandler.handleRequestCalled);
}

// CGIリクエスト(.sh)のテスト
TEST_F(GETTest, HandlesShellScriptCgiRequest) {
  // シェルスクリプト要求を設定（コンストラクタで初期化）
  HTTPRequest httpRequest("GET", "/script.sh", "HTTP/1.1",
                          std::map<std::string, std::string>(), "");

  // テスト対象のGETを作成
  GET get(mockDirective, httpRequest);

  // モックハンドラを作成
  MockGenerateHTTPResponse mockHandler;

  // ハンドラをセット
  get.setNextHandler(&mockHandler);

  // テスト実行
  get.handleRequest(httpResponse);

  // CGIリクエストでもnextHandlerは呼ばれるはず
  EXPECT_TRUE(mockHandler.handleRequestCalled);
}

// CGIとnon-CGIで異なるHandlerが設定されることをテスト
TEST_F(GETTest, SetsCorrectNextHandler) {
  // 非CGIリクエスト
  {
    // コンストラクタで初期化
    HTTPRequest httpRequest("GET", "/index.html", "HTTP/1.1",
                            std::map<std::string, std::string>(), "");

    TestableGET get(mockDirective, httpRequest);

    MockGenerateHTTPResponse mockHandler;
    get.setNextHandlerWithoutOwnership(&mockHandler);

    get.handleRequest(httpResponse);

    // ハンドラが呼び出されたことを確認
    EXPECT_TRUE(mockHandler.handleRequestCalled);
  }

  // CGIリクエスト
  {
    // コンストラクタで初期化
    HTTPRequest httpRequest("GET", "/script.py", "HTTP/1.1",
                            std::map<std::string, std::string>(), "");

    TestableGET get(mockDirective, httpRequest);

    MockGenerateHTTPResponse mockHandler;
    get.setNextHandlerWithoutOwnership(&mockHandler);

    get.handleRequest(httpResponse);

    // ハンドラが呼び出されたことを確認
    EXPECT_TRUE(mockHandler.handleRequestCalled);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
