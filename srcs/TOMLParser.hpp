#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "Directive.hpp"
#include "TOMLException.hpp"

class TOMLParser {
 public:
  TOMLParser();
  ~TOMLParser();

  // ファイルからパース（エラー時はNULLを返す）
  Directive* parseFromFile(const std::string& filename);

  // 文字列からパース（エラー時はNULLを返す）
  Directive* parseFromString(const std::string& content);

 private:
  // セクション名とセクションパスを表現する構造体
  struct SectionPath {
    std::vector<std::string> components;

    SectionPath() {}

    void clear() { components.clear(); }

    void addComponent(const std::string& component) {
      components.push_back(component);
    }

    std::string toString() const {
      std::string result;
      for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) {
          result += ".";
        }
        result += components[i];
      }
      return result;
    }
  };

  // トークンタイプを表す列挙型
  enum TokenType {
    TOKEN_NONE,
    TOKEN_SECTION_START,
    TOKEN_SECTION_END,
    TOKEN_KEY,
    TOKEN_EQUAL,
    TOKEN_VALUE,
    TOKEN_ARRAY_START,
    TOKEN_ARRAY_END,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_STRING,
    TOKEN_EOF
  };

  // トークンを表す構造体
  struct Token {
    TokenType type;
    std::string value;
    int line;

    Token() : type(TOKEN_NONE), line(1) {}
    Token(TokenType t, const std::string& v, int l)
        : type(t), value(v), line(l) {}
  };

  // 解析に必要なメソッド
  bool parseLines(std::istream& stream, Directive& rootDirective);
  bool parseSectionHeader(const std::string& line, SectionPath& currentPath,
                          int lineNum);
  bool parseKeyValue(const std::string& line, Directive& directive,
                     int lineNum);
  std::vector<std::string> parseArray(const std::string& arrayStr, int lineNum,
                                      bool& error);
  std::string parseString(const std::string& str, size_t& pos, int lineNum,
                          bool& error);

  // エスケープシーケンス処理
  std::string unescapeString(const std::string& str, int lineNum, bool& error);

  // ディレクティブ構築処理
  Directive& createNestedDirectives(Directive& rootDirective,
                                    const SectionPath& path);

  // 文字列処理ヘルパー
  std::string trim(const std::string& str) const;
  bool startsWith(const std::string& str, const std::string& prefix) const;
  bool isWhitespace(char c) const;

  // バリデーションチェック
  bool isValidKey(const std::string& key) const;
  bool hasDuplicateKeys(const Directive& directive,
                        const std::string& key) const;
  bool isTableArrayOrInlineTable(const std::string& line) const;
  bool hasMultipleKeyValuesOnLine(const std::string& line) const;
  bool isSectionOrKeyConflict(const Directive& directive,
                              const SectionPath& path) const;
  bool isDuplicateSection(const std::set<std::string>& sections,
                          const std::string& sectionPath) const;

  // エラー状態を記録するためのフラグ
  bool m_hasError;
  std::set<std::string> m_sections;
};
