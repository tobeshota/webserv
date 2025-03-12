#include "TOMLParser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

TOMLParser::TOMLParser() {}

TOMLParser::~TOMLParser() {}

Directive TOMLParser::parseFromFile(const std::string& filename) {
  std::ifstream file(filename.c_str());
  if (!file) {
    throw TOMLException("Failed to open file: " + filename);
  }

  Directive rootDirective("root");
  parseLines(file, rootDirective);
  return rootDirective;
}

Directive TOMLParser::parseFromString(const std::string& content) {
  std::istringstream stream(content);
  Directive rootDirective("root");
  parseLines(stream, rootDirective);
  return rootDirective;
}

void TOMLParser::parseLines(std::istream& stream, Directive& rootDirective) {
  std::string line;
  int lineNum = 0;
  SectionPath currentPath;

  // 最初はHTTPセクションを想定
  currentPath.addComponent("http");

  while (std::getline(stream, line)) {
    lineNum++;

    // 空行とコメント行はスキップ
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    // セクション行の解析
    if (trimmed[0] == '[' && trimmed[trimmed.size() - 1] == ']') {
      parseSectionHeader(trimmed, currentPath, lineNum);
    }
    // キーと値の解析
    else if (trimmed.find('=') != std::string::npos) {
      Directive& sectionDirective =
          createNestedDirectives(rootDirective, currentPath);
      parseKeyValue(trimmed, sectionDirective, lineNum);
    }
    // その他の行は無視
    else {
      // サポートしていない要素は無視する
    }
  }
}

void TOMLParser::parseSectionHeader(const std::string& line,
                                    SectionPath& currentPath, int lineNum) {
  // [section.subsection] 形式からセクション名を抽出
  std::string sectionName = line.substr(1, line.size() - 2);
  sectionName = trim(sectionName);

  // セクションパスをリセット
  currentPath.clear();

  // セクション名を分解
  std::string component;
  bool inQuote = false;
  char quoteChar = '\0';

  for (size_t i = 0; i < sectionName.size(); ++i) {
    char c = sectionName[i];

    if (!inQuote && (c == '"' || c == '\'')) {
      inQuote = true;
      quoteChar = c;
    } else if (inQuote && c == quoteChar) {
      inQuote = false;
      quoteChar = '\0';
    } else if (!inQuote && c == '.') {
      currentPath.addComponent(trim(component));
      component.clear();
    } else {
      component += c;
    }
  }

  if (!component.empty()) {
    currentPath.addComponent(trim(component));
  }

  // 引用符が閉じられていない場合はエラー
  if (inQuote) {
    throw TOMLException("Unclosed quote in section header", lineNum);
  }

  // セクションパスが空の場合はエラー
  if (currentPath.components.empty()) {
    throw TOMLException("Empty section name", lineNum);
  }
}

void TOMLParser::parseKeyValue(const std::string& line, Directive& directive,
                               int lineNum) {
  size_t equalPos = line.find('=');
  if (equalPos == std::string::npos) {
    throw TOMLException("Invalid key-value format", lineNum);
  }

  std::string key = trim(line.substr(0, equalPos));
  std::string valueStr = trim(line.substr(equalPos + 1));

  if (key.empty()) {
    throw TOMLException("Empty key", lineNum);
  }

  // 配列の解析
  if (valueStr[0] == '[' && valueStr[valueStr.size() - 1] == ']') {
    std::vector<std::string> values = parseArray(valueStr, lineNum);
    for (size_t i = 0; i < values.size(); ++i) {
      directive.addKeyValue(key, values[i]);
    }
  } else {
    // 文字列値の解析
    if ((valueStr[0] == '"' && valueStr[valueStr.size() - 1] == '"') ||
        (valueStr[0] == '\'' && valueStr[valueStr.size() - 1] == '\'')) {
      valueStr = valueStr.substr(1, valueStr.size() - 2);
      valueStr = unescapeString(valueStr, lineNum);
    }
    directive.addKeyValue(key, valueStr);
  }
}

std::vector<std::string> TOMLParser::parseArray(const std::string& arrayStr,
                                                int lineNum) {
  std::vector<std::string> result;
  std::string content = arrayStr.substr(1, arrayStr.size() - 2);

  size_t pos = 0;
  while (pos < content.size()) {
    // ホワイトスペースをスキップ
    while (pos < content.size() && isWhitespace(content[pos])) {
      ++pos;
    }

    if (pos < content.size()) {
      if (content[pos] == '"' || content[pos] == '\'') {
        // 文字列の解析
        std::string value = parseString(content, pos, lineNum);
        result.push_back(value);
      } else {
        // 文字列以外（数値など）
        size_t commaPos = content.find(',', pos);
        if (commaPos == std::string::npos) {
          commaPos = content.size();
        }
        std::string value = trim(content.substr(pos, commaPos - pos));
        result.push_back(value);
        pos = commaPos;
      }

      // 次の要素へ
      while (pos < content.size() && content[pos] != ',') {
        ++pos;
      }
      if (pos < content.size()) {
        ++pos;  // カンマをスキップ
      }
    }
  }

  return result;
}

std::string TOMLParser::parseString(const std::string& str, size_t& pos,
                                    int lineNum) {
  char quoteChar = str[pos];
  ++pos;  // 開き引用符をスキップ

  std::string result;
  bool escaped = false;

  while (pos < str.size()) {
    char c = str[pos];

    if (escaped) {
      // エスケープシーケンス処理
      switch (c) {
        case '"':
          result += '"';
          break;
        case '\'':
          result += '\'';
          break;
        case '\\':
          result += '\\';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        default:
          throw TOMLException("Invalid escape sequence", lineNum);
      }
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == quoteChar) {
      ++pos;  // 閉じ引用符をスキップ
      return result;
    } else {
      result += c;
    }

    ++pos;
  }

  throw TOMLException("Unclosed string literal", lineNum);
}

std::string TOMLParser::unescapeString(const std::string& str, int lineNum) {
  std::string result;
  bool escaped = false;

  for (size_t i = 0; i < str.size(); ++i) {
    char c = str[i];

    if (escaped) {
      // エスケープシーケンス処理
      switch (c) {
        case '"':
          result += '"';
          break;
        case '\'':
          result += '\'';
          break;
        case '\\':
          result += '\\';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        default:
          throw TOMLException("Invalid escape sequence", lineNum);
      }
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else {
      result += c;
    }
  }

  // 最後がエスケープシーケンスで終わっている場合
  if (escaped) {
    throw TOMLException("Invalid escape sequence at end of string", lineNum);
  }

  return result;
}

Directive& TOMLParser::createNestedDirectives(Directive& rootDirective,
                                              const SectionPath& path) {
  Directive* current = &rootDirective;

  for (size_t i = 0; i < path.components.size(); ++i) {
    const std::string& component = path.components[i];
    bool found = false;

    // 子ディレクティブの中から同じ名前のものを探す
    for (size_t j = 0; j < current->children().size(); ++j) {
      if (current->children()[j].name() == component) {
        current = &current->children()[j];
        found = true;
        break;
      }
    }

    // なければ新しく作成
    if (!found) {
      current->addChild(Directive(component));
      current = &current->children()[current->children().size() - 1];
    }
  }

  return *current;
}

std::string TOMLParser::trim(const std::string& str) const {
  size_t start = 0;
  size_t end = str.size();

  // 先頭の空白をスキップ
  while (start < end && isWhitespace(str[start])) {
    ++start;
  }

  // 末尾の空白をスキップ
  while (start < end && isWhitespace(str[end - 1])) {
    --end;
  }

  if (start >= end) {
    return "";
  }

  return str.substr(start, end - start);
}

bool TOMLParser::startsWith(const std::string& str,
                            const std::string& prefix) const {
  if (str.size() < prefix.size()) {
    return false;
  }

  for (size_t i = 0; i < prefix.size(); ++i) {
    if (str[i] != prefix[i]) {
      return false;
    }
  }

  return true;
}

bool TOMLParser::isWhitespace(char c) const {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
