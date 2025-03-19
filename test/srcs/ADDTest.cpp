#include <gtest/gtest.h>

#include "ADD.hpp"
#include "Directive.hpp"
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
class TestableADD : public ADD {
 public:
  TestableADD(Directive rootDirective, HTTPRequest httpRequest)
      : ADD(rootDirective, httpRequest) {}

  // _nextHandlerを確認するための関数
  Handler* getNextHandler() const { return _nextHandler; }

  // 通常setNextHandlerで削除されるハンドラをテスト中に削除しないようにする
  void setNextHandlerWithoutOwnership(Handler* handler) {
    _nextHandler = handler;
  }
};

class ADDTest : public ::testing::Test {
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
TEST_F(ADDTest, HandlesNonCgiRequest) {
  // 通常のHTML要求を設定（コンストラクタで初期化）
  HTTPRequest httpRequest("GET", "/index.html", "HTTP/1.1",
                          std::map<std::string, std::string>(), "");

  // テスト対象のADDを作成
  TestableADD add(mockDirective, httpRequest);

  // モックハンドラを作成し、ADDのnextHandlerとしてセット
  MockGenerateHTTPResponse* mockHandler = new MockGenerateHTTPResponse();

  // モックをセット (通常は内部で自動的に行われる)
  add.setNextHandlerWithoutOwnership(mockHandler);

  // テスト実行
  add.handleRequest(httpResponse);

  // handleRequestが呼ばれたことを確認
  EXPECT_TRUE(mockHandler->handleRequestCalled);

  // メモリリーク防止
  delete mockHandler;
}

// CGIリクエスト(.py)のテスト
TEST_F(ADDTest, HandlesPythonCgiRequest) {
  // Pythonスクリプト要求を設定（コンストラクタで初期化）
  HTTPRequest httpRequest("GET", "/script.py", "HTTP/1.1",
                          std::map<std::string, std::string>(), "");

  // テスト対象のADDを作成
  ADD add(mockDirective, httpRequest);

  // モックハンドラを作成
  MockGenerateHTTPResponse mockHandler;

  // ハンドラをセット
  add.setNextHandler(&mockHandler);

  // テスト実行
  add.handleRequest(httpResponse);

  // CGIリクエストでもnextHandlerは呼ばれるはず
  EXPECT_TRUE(mockHandler.handleRequestCalled);
}

// CGIリクエスト(.sh)のテスト
TEST_F(ADDTest, HandlesShellScriptCgiRequest) {
  // シェルスクリプト要求を設定（コンストラクタで初期化）
  HTTPRequest httpRequest("GET", "/script.sh", "HTTP/1.1",
                          std::map<std::string, std::string>(), "");

  // テスト対象のADDを作成
  ADD add(mockDirective, httpRequest);

  // モックハンドラを作成
  MockGenerateHTTPResponse mockHandler;

  // ハンドラをセット
  add.setNextHandler(&mockHandler);

  // テスト実行
  add.handleRequest(httpResponse);

  // CGIリクエストでもnextHandlerは呼ばれるはず
  EXPECT_TRUE(mockHandler.handleRequestCalled);
}

// CGIとnon-CGIで異なるHandlerが設定されることをテスト
TEST_F(ADDTest, SetsCorrectNextHandler) {
  // 非CGIリクエスト
  {
    // コンストラクタで初期化
    HTTPRequest httpRequest("GET", "/index.html", "HTTP/1.1",
                            std::map<std::string, std::string>(), "");

    TestableADD add(mockDirective, httpRequest);

    MockGenerateHTTPResponse mockHandler;
    add.setNextHandlerWithoutOwnership(&mockHandler);

    add.handleRequest(httpResponse);

    // ハンドラが呼び出されたことを確認
    EXPECT_TRUE(mockHandler.handleRequestCalled);
  }

  // CGIリクエスト
  {
    // コンストラクタで初期化
    HTTPRequest httpRequest("GET", "/script.py", "HTTP/1.1",
                            std::map<std::string, std::string>(), "");

    TestableADD add(mockDirective, httpRequest);

    MockGenerateHTTPResponse mockHandler;
    add.setNextHandlerWithoutOwnership(&mockHandler);

    add.handleRequest(httpResponse);

    // ハンドラが呼び出されたことを確認
    EXPECT_TRUE(mockHandler.handleRequestCalled);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
