#include "ListenDirectory.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>  // アクセス権限チェック用

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

// ディレクトリエントリの情報を格納する構造体
struct DirectoryEntry {
  std::string name;
  bool isDirectory;

  // ソート用の比較演算子
  bool operator<(const DirectoryEntry& other) const {
    // ディレクトリを先に表示
    if (isDirectory != other.isDirectory)
      return isDirectory > other.isDirectory;
    // 同じタイプならアルファベット順
    return name < other.name;
  }
};

// 整数を文字列に変換
std::string int2str(size_t value) {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

// コンストラクタの実装
ListenDirectory::ListenDirectory(const std::string& dirpath)
    : _dirpath(dirpath) {}

void ListenDirectory::handleRequest(HTTPResponse& httpResponse) {
  // 以前はhttpResponseからディレクトリパスを取得していましたが、
  // 今はコンストラクタで設定された_dirpathを使用します
  std::string dirPath = _dirpath;

  // ディレクトリの読み取り権限と実行権限を確認
  if (access(dirPath.c_str(), R_OK | X_OK) != 0) {
    // 権限がない場合は空のボディを設定
    httpResponse.setHttpResponseBody("");
    return;
  }

  // ディレクトリが存在するか確認
  DIR* dir = opendir(dirPath.c_str());
  if (!dir) {
    // ディレクトリが開けない場合は空のボディを設定
    httpResponse.setHttpResponseBody("");
    return;
  }

  // ディレクトリエントリの収集
  std::vector<DirectoryEntry> entries;
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    // "."は表示しない（".."は親ディレクトリへのリンクとして表示）
    if (std::string(entry->d_name) == ".") continue;

    DirectoryEntry dirEntry;
    dirEntry.name = entry->d_name;

    // ファイル情報の取得
    struct stat statBuf;
    std::string fullPath = dirPath + "/" + entry->d_name;
    if (stat(fullPath.c_str(), &statBuf) == 0) {
      dirEntry.isDirectory = S_ISDIR(statBuf.st_mode);
    } else {
      dirEntry.isDirectory = false;
    }

    entries.push_back(dirEntry);
  }
  closedir(dir);

  // エントリをソート
  std::sort(entries.begin(), entries.end());

  // HTMLの生成
  std::stringstream html;
  html << "<!DOCTYPE html>\n"
       << "<html>\n"
       << "<head>\n"
       << "    <meta charset=\"utf-8\">\n"
       << "    <title>Directory listing for " << dirPath << "</title>\n"
       << "    <style>\n"
       << "        body { font-family: Arial, sans-serif; margin: 0; padding: "
          "20px; }\n"
       << "        h1 { border-bottom: 1px solid #ddd; margin-bottom: 20px; "
          "padding-bottom: 10px; }\n"
       << "        table { border-collapse: collapse; width: 100%; }\n"
       << "        th, td { text-align: left; padding: 8px; }\n"
       << "        tr:nth-child(even) { background-color: #f2f2f2; }\n"
       << "        th { background-color: #4CAF50; color: white; }\n"
       << "        a { text-decoration: none; color: #0066cc; }\n"
       << "        a:hover { text-decoration: underline; }\n"
       << "    </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "    <h1>Directory listing for " << dirPath << "</h1>\n"
       << "    <table>\n"
       << "        <tr>\n"
       << "            <th>Name</th>\n"
       << "        </tr>\n";

  // 親ディレクトリへのリンク
  html << "        <tr>\n"
       << "            <td><a href=\"../\">Parent Directory</a></td>\n"
       << "        </tr>\n";

  // ディレクトリとファイルのエントリ
  for (size_t i = 0; i < entries.size(); ++i) {
    const DirectoryEntry& entry = entries[i];
    html << "        <tr>\n"
         << "            <td><a href=\"" << entry.name;

    // ディレクトリの場合は末尾に/を追加
    if (entry.isDirectory) html << "/";

    html << "\">" << entry.name;

    // ディレクトリの場合は末尾に/を追加
    if (entry.isDirectory) html << "/";

    html << "</a></td>\n"
         << "        </tr>\n";
  }

  html << "    </table>\n"
       << "</body>\n"
       << "</html>";

  // レスポンスの設定（ボディのみ）
  std::string htmlContent = html.str();
  httpResponse.setHttpResponseBody(htmlContent);

  // 連鎖的なハンドラ処理
  if (_nextHandler) _nextHandler->handleRequest(httpResponse);
}
