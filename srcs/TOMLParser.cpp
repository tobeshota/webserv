#include "TOMLParser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

TOMLParser::TOMLParser() : m_hasError(false) {}

TOMLParser::~TOMLParser() {}

Directive* TOMLParser::parseFromFile(const std::string& filename) {
  std::ifstream file(filename.c_str());
  if (!file) {
    return NULL;
  }

  m_hasError = false;
  m_sections.clear();

  Directive* rootDirective = new Directive("root");
  if (!parseLines(file, *rootDirective)) {
    delete rootDirective;
    return NULL;
  }
  return rootDirective;
}

Directive* TOMLParser::parseFromString(const std::string& content) {
  std::istringstream stream(content);

  m_hasError = false;
  m_sections.clear();

  Directive* rootDirective = new Directive("root");
  if (!parseLines(stream, *rootDirective)) {
    delete rootDirective;
    return NULL;
  }
  return rootDirective;
}

bool TOMLParser::parseLines(std::istream& stream, Directive& rootDirective) {
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

    // インラインテーブルやテーブル配列のチェック
    if (isTableArrayOrInlineTable(trimmed)) {
      return false;
    }

    // 1行に複数のキーと値があるかチェック
    if (hasMultipleKeyValuesOnLine(trimmed)) {
      return false;
    }

    // セクション行の解析
    if (trimmed[0] == '[' && trimmed[trimmed.size() - 1] == ']') {
      if (!parseSectionHeader(trimmed, currentPath, lineNum)) {
        return false;
      }

      // セクションパスが重複していないかチェック
      std::string sectionPathStr = currentPath.toString();
      if (isDuplicateSection(m_sections, sectionPathStr)) {
        return false;
      }
      m_sections.insert(sectionPathStr);

      // セクションとキーの名前衝突をチェック
      if (isSectionOrKeyConflict(rootDirective, currentPath)) {
        return false;
      }
    }
    // キーと値の解析
    else if (trimmed.find('=') != std::string::npos) {
      Directive& sectionDirective =
          createNestedDirectives(rootDirective, currentPath);
      if (!parseKeyValue(trimmed, sectionDirective, lineNum)) {
        return false;
      }
    } else {
      // 無効な形式の行
      return false;
    }
  }

  return !m_hasError;
}

bool TOMLParser::parseSectionHeader(const std::string& line,
                                    SectionPath& currentPath, int lineNum) {
  // 未使用パラメータをvoidキャスト
  (void)lineNum;

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
    return false;
  }

  // セクションパスが空の場合はエラー
  if (currentPath.components.empty()) {
    return false;
  }

  return true;
}

bool TOMLParser::parseKeyValue(const std::string& line, Directive& directive,
                               int lineNum) {
  size_t equalPos = line.find('=');
  if (equalPos == std::string::npos) {
    return false;
  }

  std::string key = trim(line.substr(0, equalPos));
  std::string valueStr = trim(line.substr(equalPos + 1));

  // キーが空または有効でない場合はエラー
  if (key.empty() || !isValidKey(key)) {
    return false;
  }

  // 値が空の場合はエラー
  if (valueStr.empty()) {
    return false;
  }

  // 同じキーが既に存在するかチェック
  if (hasDuplicateKeys(directive, key)) {
    return false;
  }

  bool error = false;

  // 配列の解析
  if (valueStr[0] == '[' && valueStr[valueStr.size() - 1] == ']') {
    std::vector<std::string> values = parseArray(valueStr, lineNum, error);
    if (error) {
      return false;
    }
    for (size_t i = 0; i < values.size(); ++i) {
      directive.addKeyValue(key, values[i]);
    }
  } else {
    // 文字列値の解析
    if ((valueStr[0] == '"' && valueStr[valueStr.size() - 1] == '"') ||
        (valueStr[0] == '\'' && valueStr[valueStr.size() - 1] == '\'')) {
      valueStr = valueStr.substr(1, valueStr.size() - 2);
      valueStr = unescapeString(valueStr, lineNum, error);
      if (error) {
        return false;
      }
    }
    directive.addKeyValue(key, valueStr);
  }

  return true;
}

std::vector<std::string> TOMLParser::parseArray(const std::string& arrayStr,
                                                int lineNum, bool& error) {
  std::vector<std::string> result;
  std::string content = arrayStr.substr(1, arrayStr.size() - 2);

  // 配列内に改行があるかチェック
  if (content.find('\n') != std::string::npos) {
    error = true;
    return result;
  }

  size_t pos = 0;
  while (pos < content.size()) {
    // ホワイトスペースをスキップ
    while (pos < content.size() && isWhitespace(content[pos])) {
      ++pos;
    }

    if (pos < content.size()) {
      if (content[pos] == '"' || content[pos] == '\'') {
        // 文字列の解析
        std::string value = parseString(content, pos, lineNum, error);
        if (error) {
          return result;
        }
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
                                    int lineNum, bool& error) {
  // 未使用パラメータをvoidキャスト
  (void)lineNum;

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
          error = true;
          return "";
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

  error = true;
  return "";
}

std::string TOMLParser::unescapeString(const std::string& str, int lineNum,
                                       bool& error) {
  // 未使用パラメータをvoidキャスト
  (void)lineNum;

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
          error = true;
          return "";
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
    error = true;
    return "";
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

// キーが有効かチェック（A-Za-z0-9のみ許容）
bool TOMLParser::isValidKey(const std::string& key) const {
  std::string unquotedKey = key;

  // 引用符で囲まれたキーの場合、引用符を取り除く
  if ((key.size() >= 2 && key[0] == '"' && key[key.size() - 1] == '"') ||
      (key.size() >= 2 && key[0] == '\'' && key[key.size() - 1] == '\'')) {
    unquotedKey = key.substr(1, key.size() - 2);
  }

  for (size_t i = 0; i < unquotedKey.size(); ++i) {
    char c = unquotedKey[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '_')) {
      return false;
    }
  }
  return true;
}

// 同じキーが存在するかチェック
bool TOMLParser::hasDuplicateKeys(const Directive& directive,
                                  const std::string& key) const {
  // キーが引用符で囲まれている場合は、引用符なしの同じキーと比較する
  std::string unquotedKey = key;
  if ((key.size() >= 2 && key[0] == '"' && key[key.size() - 1] == '"') ||
      (key.size() >= 2 && key[0] == '\'' && key[key.size() - 1] == '\'')) {
    unquotedKey = key.substr(1, key.size() - 2);
  }

  const std::map<std::string, std::vector<std::string> >& keyValues =
      directive.keyValues();
  for (std::map<std::string, std::vector<std::string> >::const_iterator it =
           keyValues.begin();
       it != keyValues.end(); ++it) {
    std::string existingKey = it->first;

    // 既存キーの引用符も取り除いて比較
    if ((existingKey.size() >= 2 && existingKey[0] == '"' &&
         existingKey[existingKey.size() - 1] == '"') ||
        (existingKey.size() >= 2 && existingKey[0] == '\'' &&
         existingKey[existingKey.size() - 1] == '\'')) {
      existingKey = existingKey.substr(1, existingKey.size() - 2);
    }

    if (existingKey == unquotedKey) {
      return true;  // 重複あり
    }
  }

  return false;  // 重複なし
}

// テーブル配列かインラインテーブルかチェック
bool TOMLParser::isTableArrayOrInlineTable(const std::string& line) const {
  // テーブル配列 [[...]] のチェック
  if (line.size() >= 2 && line[0] == '[' && line[1] == '[') {
    return true;
  }

  // インラインテーブル {key = val} のチェック
  if (line.find('{') != std::string::npos) {
    return true;
  }

  return false;
}

// 1行に複数のキーと値のペアがあるかチェック
bool TOMLParser::hasMultipleKeyValuesOnLine(const std::string& line) const {
  bool inQuote = false;
  char quoteChar = '\0';
  size_t equalCount = 0;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    if (!inQuote && (c == '"' || c == '\'')) {
      inQuote = true;
      quoteChar = c;
    } else if (inQuote && c == quoteChar &&
               (i == 0 ||
                line[i - 1] !=
                    '\\')) {  // バックスラッシュでエスケープされていない
      inQuote = false;
    } else if (!inQuote && c == '=') {
      equalCount++;
      if (equalCount > 1) {
        return true;  // 複数の=がある
      }
    }
  }

  return false;
}

// セクションとキーの衝突をチェック
bool TOMLParser::isSectionOrKeyConflict(const Directive& directive,
                                        const SectionPath& path) const {
  Directive* current = const_cast<Directive*>(&directive);

  // ルートディレクティブを確認
  for (size_t i = 0; i < path.components.size() - 1; ++i) {
    const std::string& component = path.components[i];
    bool found = false;

    for (size_t j = 0; j < current->children().size(); ++j) {
      if (current->children()[j].name() == component) {
        current = &current->children()[j];
        found = true;
        break;
      }
    }

    if (!found) {
      return false;  // セクションがまだ存在しないので衝突なし
    }
  }

  // 最後のコンポーネントがキーとして既に存在するかチェック
  if (!path.components.empty()) {
    const std::string& lastComponent = path.components.back();
    const std::map<std::string, std::vector<std::string> >& keyValues =
        current->keyValues();

    for (std::map<std::string, std::vector<std::string> >::const_iterator it =
             keyValues.begin();
         it != keyValues.end(); ++it) {
      if (it->first == lastComponent) {
        return true;  // 衝突あり
      }
    }
  }

  return false;
}

// セクションが重複しているかチェック
bool TOMLParser::isDuplicateSection(const std::set<std::string>& sections,
                                    const std::string& sectionPath) const {
  return sections.find(sectionPath) != sections.end();
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
