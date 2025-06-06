#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

// =========================
// Directive クラス
// =========================
class Directive {
 public:
  // 同じキーに対して複数の値を持てる
  typedef std::map<std::string, std::vector<std::string> > KVMap;
  // 子ディレクティブの配列 (再帰構造)
  typedef std::vector<Directive> DirectiveList;

  Directive() {}
  explicit Directive(const std::string &name) : m_name(name) {}

  // ディレクティブ名
  void setName(const std::string &name) { m_name = name; }
  const std::string &name() const { return m_name; }

  // キー‐値を追加（同じキーがあれば複数値がたまる）
  void addKeyValue(const std::string &key, const std::string &value) {
    m_keyValues[key].push_back(value);
  }

  // キー‐値のマップ(キー -> 値たち)
  KVMap &keyValues() { return m_keyValues; }
  const KVMap &keyValues() const { return m_keyValues; }

  // 子ディレクティブを追加
  void addChild(const Directive &child) { m_children.push_back(child); }

  // 子ディレクティブ取得
  DirectiveList &children() { return m_children; }
  const DirectiveList &children() const { return m_children; }

  // 現在のディレクティブからキーに対する値を階層すべて探し取得（なければ空文字列）
  std::vector<std::string> getValues(const std::string &key,
                                     bool recursive = true) const {
    std::vector<std::string> result;
    KVMap::const_iterator it = m_keyValues.find(key);
    if (it != m_keyValues.end()) {
      result = it->second;
    }

    // 再帰検索が有効な場合のみ、子ディレクティブも探索
    if (recursive) {
      for (std::size_t i = 0; i < m_children.size(); ++i) {
        std::vector<std::string> childValues =
            m_children[i].getValues(key, recursive);
        result.insert(result.end(), childValues.begin(), childValues.end());
      }
    }
    return result;
  }

  // キーに対する値を階層すべて探し取得（最初に見つかったもの）
  std::string getValue(const std::string &key) const {
    std::vector<std::string> values = getValues(key);
    if (values.empty()) {
      return "";
    }
    return values[0];
  }

  // 指定された階層のディレクティブを探す
  const Directive *findDirective(
      const std::vector<std::string> &dirNames) const {
    if (dirNames.empty()) {
      return NULL;
    }

    // 最初の名前に一致する子ディレクティブを探す
    for (std::size_t i = 0; i < m_children.size(); ++i) {
      if (m_children[i].name() == dirNames[0]) {
        // これが最後の名前なら、その子ディレクティブを返す
        if (dirNames.size() == 1) {
          return &m_children[i];
        }

        // 残りの階層を探索するために、ベクトルの残りを渡す
        std::vector<std::string> remainingNames(dirNames.begin() + 1,
                                                dirNames.end());
        const Directive *result = m_children[i].findDirective(remainingNames);
        if (result) {
          return result;
        }
      }
    }

    // 見つからなかった場合はNULLを返す
    return NULL;
  }

  // C++98互換の便利メソッド - 1つのパラメータ
  const Directive *findDirective(const std::string &dir1) const {
    std::vector<std::string> dirs;
    dirs.push_back(dir1);
    return findDirective(dirs);
  }

  // C++98互換の便利メソッド - 2つのパラメータ
  const Directive *findDirective(const std::string &dir1,
                                 const std::string &dir2) const {
    std::vector<std::string> dirs;
    dirs.push_back(dir1);
    dirs.push_back(dir2);
    return findDirective(dirs);
  }

  // C++98互換の便利メソッド - 3つのパラメータ
  const Directive *findDirective(const std::string &dir1,
                                 const std::string &dir2,
                                 const std::string &dir3) const {
    std::vector<std::string> dirs;
    dirs.push_back(dir1);
    dirs.push_back(dir2);
    dirs.push_back(dir3);
    return findDirective(dirs);
  }

 private:
  std::string m_name;        // ディレクティブ名
  KVMap m_keyValues;         // キー -> 値（複数）のマップ
  DirectiveList m_children;  // 子ディレクティブ(再帰構造)
};

// =========================
// 再帰的にDirectiveを表示
// =========================
template <typename DirectiveType>
void printDirective(const DirectiveType &dir, int indent = 0) {
  // インデントしてディレクティブ名を表示
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
  std::cout << "[" << dir.name() << "]" << std::endl;

  // キー‐値の表示
  const typename DirectiveType::KVMap &map = dir.keyValues();
  for (typename DirectiveType::KVMap::const_iterator it = map.begin();
       it != map.end(); ++it) {
    const std::string &key = it->first;
    const std::vector<std::string> &vals = it->second;
    for (std::size_t i = 0; i < vals.size(); ++i) {
      // インデント
      for (int s = 0; s < indent + 1; ++s) {
        std::cout << "  ";
      }
      std::cout << key << " = " << vals[i] << std::endl;
    }
  }

  // 子ディレクティブの再帰表示
  const typename DirectiveType::DirectiveList &childs = dir.children();
  for (std::size_t i = 0; i < childs.size(); ++i) {
    printDirective(childs[i], indent + 1);
  }
}
