#include <gtest/gtest.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "TOMLParser.hpp"

class TOMLParserTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    parser = new TOMLParser();
  }

  virtual void TearDown() {
    delete parser;
  }

  TOMLParser* parser;

  // テスト用の一時ファイルを作成するヘルパー関数
  void createTempFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename.c_str());
    file << content;
    file.close();
  }

  // テスト用の一時ファイルを削除するヘルパー関数
  void removeTempFile(const std::string& filename) {
    std::remove(filename.c_str());
  }
};

// 基本的なパーステスト
TEST_F(TOMLParserTest, BasicParsing) {
  std::string content = "[http]\nserver_name = webserv\nport = 8080\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].name(), "http");
  EXPECT_EQ(directive->children()[0].keyValues().size(), 2);
  EXPECT_EQ(directive->children()[0].keyValues().at("server_name")[0], "webserv");
  EXPECT_EQ(directive->children()[0].keyValues().at("port")[0], "8080");
  
  delete directive;
}

// ファイルからのパーステスト
TEST_F(TOMLParserTest, ParseFromFile) {
  const std::string filename = "temp_toml_test.toml";
  std::string content = "[http]\nserver_name = webserv\nport = 8080\n";
  
  createTempFile(filename, content);
  
  Directive* directive = parser->parseFromFile(filename);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].name(), "http");
  EXPECT_EQ(directive->children()[0].keyValues().size(), 2);
  EXPECT_EQ(directive->children()[0].keyValues().at("server_name")[0], "webserv");
  EXPECT_EQ(directive->children()[0].keyValues().at("port")[0], "8080");
  
  delete directive;
  removeTempFile(filename);
}

// 存在しないファイルのテスト
TEST_F(TOMLParserTest, NonExistentFile) {
  Directive* directive = parser->parseFromFile("non_existent_file.toml");
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 空のコンテンツのテスト
TEST_F(TOMLParserTest, EmptyContent) {
  std::string content = "";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 0);
  
  delete directive;
}

// コメント行のテスト
TEST_F(TOMLParserTest, CommentLines) {
  std::string content = "[http]\n# This is a comment\nserver_name = webserv\n# Another comment\nport = 8080\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].name(), "http");
  EXPECT_EQ(directive->children()[0].keyValues().size(), 2);
  EXPECT_EQ(directive->children()[0].keyValues().at("server_name")[0], "webserv");
  EXPECT_EQ(directive->children()[0].keyValues().at("port")[0], "8080");
  
  delete directive;
}

// ネストされたセクションのテスト
TEST_F(TOMLParserTest, NestedSections) {
  std::string content = "[http]\n[http.server]\nname = \"main_server\"\n[http.server.location]\npath = \"/\"\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].name(), "http");
  EXPECT_EQ(directive->children()[0].children().size(), 1);
  EXPECT_EQ(directive->children()[0].children()[0].name(), "server");
  EXPECT_EQ(directive->children()[0].children()[0].keyValues().at("name")[0], "main_server");
  EXPECT_EQ(directive->children()[0].children()[0].children().size(), 1);
  EXPECT_EQ(directive->children()[0].children()[0].children()[0].name(), "location");
  EXPECT_EQ(directive->children()[0].children()[0].children()[0].keyValues().at("path")[0], "/");
  
  delete directive;
}

// 引用符付き文字列のテスト
TEST_F(TOMLParserTest, QuotedStrings) {
  std::string content = "[http]\nserver_name = \"my webserv\"\nroot = '/var/www'\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].keyValues().at("server_name")[0], "my webserv");
  EXPECT_EQ(directive->children()[0].keyValues().at("root")[0], "/var/www");
  
  delete directive;
}

// エスケープシーケンスを含む文字列のテスト
TEST_F(TOMLParserTest, EscapeSequences) {
  std::string content = "[http]\nmessage = \"Hello\\nWorld\"\npath = \"C:\\\\Program Files\"\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].keyValues().at("message")[0], "Hello\nWorld");
  EXPECT_EQ(directive->children()[0].keyValues().at("path")[0], "C:\\Program Files");
  
  delete directive;
}

// 配列のテスト
TEST_F(TOMLParserTest, Arrays) {
  std::string content = "[http]\nallowed_methods = [\"GET\", \"POST\", \"DELETE\"]\nports = [8080, 8081]\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].keyValues().at("allowed_methods").size(), 3);
  EXPECT_EQ(directive->children()[0].keyValues().at("allowed_methods")[0], "GET");
  EXPECT_EQ(directive->children()[0].keyValues().at("allowed_methods")[1], "POST");
  EXPECT_EQ(directive->children()[0].keyValues().at("allowed_methods")[2], "DELETE");
  EXPECT_EQ(directive->children()[0].keyValues().at("ports").size(), 2);
  EXPECT_EQ(directive->children()[0].keyValues().at("ports")[0], "8080");
  EXPECT_EQ(directive->children()[0].keyValues().at("ports")[1], "8081");
  
  delete directive;
}

// 空配列のテスト
// TEST_F(TOMLParserTest, EmptyArray) {
//   std::string content = "[http]\nempty_array = []\n";
//   Directive* directive = parser->parseFromString(content);
  
//   ASSERT_NE(directive, (Directive*)NULL);
//   EXPECT_EQ(directive->children().size(), 1);
//   EXPECT_EQ(directive->children()[0].keyValues().at("empty_array").size(), 0);
  
//   delete directive;
// }

// 無効なTOML構文のテスト
TEST_F(TOMLParserTest, InvalidSyntax) {
  // 等号がない行
  std::string content = "[http]\nserver_name webserv\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 無効なセクションヘッダーのテスト
TEST_F(TOMLParserTest, InvalidSectionHeader) {
  // 閉じ括弧がない
  std::string content = "[http\nserver_name = webserv\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 無効な配列のテスト
// TEST_F(TOMLParserTest, InvalidArray) {
//   // 閉じ括弧がない
//   std::string content = "[http]\nports = [8080, 8081\n";
//   Directive* directive = parser->parseFromString(content);
//   EXPECT_EQ(directive, (Directive*)NULL);
// }

// 無効な文字列リテラルのテスト
// TEST_F(TOMLParserTest, InvalidStringLiteral) {
//   // 閉じ引用符がない
//   std::string content = "[http]\nserver_name = \"webserv\n";
//   Directive* directive = parser->parseFromString(content);
//   EXPECT_EQ(directive, (Directive*)NULL);
// }

// 重複キーのテスト
TEST_F(TOMLParserTest, DuplicateKey) {
  std::string content = "[http]\nserver_name = webserv\nserver_name = another\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 重複セクションのテスト
TEST_F(TOMLParserTest, DuplicateSection) {
  std::string content = "[http]\nserver_name = webserv\n[http]\nport = 8080\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// テーブル配列の拒否テスト
TEST_F(TOMLParserTest, TableArrayRejection) {
  std::string content = "[[http]]\nserver_name = webserv\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// インラインテーブルの拒否テスト
TEST_F(TOMLParserTest, InlineTableRejection) {
  std::string content = "[http]\nserver = { name = \"webserv\", port = 8080 }\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// セクションとキーの衝突テスト
TEST_F(TOMLParserTest, SectionKeyConflict) {
  std::string content = "[http]\nserver = value\n[http.server]\nname = \"main_server\"\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 複数キー・値のテスト
TEST_F(TOMLParserTest, MultipleKeyValuePairs) {
  std::string content = "[http]\nserver_name = webserv port = 8080\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 空のキーテスト
TEST_F(TOMLParserTest, EmptyKey) {
  std::string content = "[http]\n = value\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 空のセクション名テスト
TEST_F(TOMLParserTest, EmptySectionName) {
  std::string content = "[]\nkey = value\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 無効なキー名テスト
TEST_F(TOMLParserTest, InvalidKeyName) {
  std::string content = "[http]\nserver-name = webserv\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 無効なエスケープシーケンステスト
TEST_F(TOMLParserTest, InvalidEscapeSequence) {
  std::string content = "[http]\nserver_name = \"web\\xserv\"\n";
  Directive* directive = parser->parseFromString(content);
  EXPECT_EQ(directive, (Directive*)NULL);
}

// 引用符付きキーテスト
TEST_F(TOMLParserTest, QuotedKeys) {
  std::string content = "[http]\n\"server_name\" = webserv\n'port' = 8080\n";
  Directive* directive = parser->parseFromString(content);
  
  ASSERT_NE(directive, (Directive*)NULL);
  EXPECT_EQ(directive->children().size(), 1);
  EXPECT_EQ(directive->children()[0].keyValues().at("\"server_name\"")[0], "webserv");
  EXPECT_EQ(directive->children()[0].keyValues().at("'port'")[0], "8080");
  
  delete directive;
}
