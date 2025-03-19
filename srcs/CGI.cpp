#include "CGI.hpp"

#include <dirent.h>  // ディレクトリの内容を読み取るために追加
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>  // select() 用に追加
#include <sys/wait.h>

#include <cstdlib>
#include <cstring>
#include <ctime>  // time() 用に追加
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

CGI::CGI(Directive rootDirective, HTTPRequest httpRequest)
    : _rootDirective(rootDirective), _httpRequest(httpRequest) {}

CGI::~CGI() {}

bool CGI::isPythonScript(const std::string& url) const {
  // クエリパラメータを削除
  std::string cleanUrl = url;
  size_t queryPos = cleanUrl.find('?');
  if (queryPos != std::string::npos) {
    cleanUrl = cleanUrl.substr(0, queryPos);
  }

  // ディレクトリの場合はさらに判断
  if (!cleanUrl.empty() && cleanUrl[cleanUrl.length() - 1] == '/') {
    return true;  // ディレクトリの場合も処理対象として認識
  }

  // URLの拡張子が.pyかどうかをチェック
  size_t dotPos = cleanUrl.find_last_of('.');
  if (dotPos != std::string::npos) {
    std::string extension = cleanUrl.substr(dotPos);
    return extension == ".py";
  }
  return false;
}

static bool isDirectory(const std::string& filePath) {
  // 文字列が空でないかつ最後の文字が '/' であるかを確認
  if (!filePath.empty() && filePath[filePath.length() - 1] == '/') {
    return true;
  }
  return false;
}

std::string CGI::getScriptPath() const {
  std::string url = _httpRequest.getURL();
  std::string rootValue = "";  // デフォルト値を空に

  // クエリパラメータを削除
  size_t queryPos = url.find('?');
  if (queryPos != std::string::npos) {
    url = url.substr(0, queryPos);
  }

  // ホストディレクティブからrootの値を取得
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getHeader("Host"));
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  // URLがディレクトリの場合（GenerateHTTPResponseと同様の処理）
  if (isDirectory(url)) {
    // インデックスファイルを探す
    const Directive* indexDirective = _rootDirective.findDirective(
        _httpRequest.getHeader("Host"), "location", url);
    if (indexDirective != NULL) {
      std::string indexValue = indexDirective->getValue("index");
      if (!indexValue.empty() && isPythonScript(indexValue)) {
        return rootValue + url + indexValue;
      }
    }
    // インデックスファイルが見つからない場合はディレクトリパスを返す
    // 実際のファイルパスではないので、後でディレクトリリスト生成処理が必要
    return rootValue + url;
  }

  // リクエストされたリソースのフルパスを返す
  return rootValue + url;
}

// ディレクトリの内容をHTMLリストとして生成する関数を追加
std::string CGI::generateDirectoryListing(const std::string& dirPath) const {
  DIR* dir = opendir(dirPath.c_str());
  if (!dir) {
    return "";
  }

  std::stringstream html;
  html << "<html>\n<head>\n<title>Directory Index</title>\n</head>\n";
  html << "<body>\n<h1>Directory Index: " << _httpRequest.getURL()
       << "</h1>\n<ul>\n";

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name != "." && name != "..") {
      html << "<li><a href=\"" << name;
      // ディレクトリなら/を追加
      if (entry->d_type == DT_DIR) {
        html << "/";
      }
      html << "\">" << name;
      if (entry->d_type == DT_DIR) {
        html << "/";
      }
      html << "</a></li>\n";
    }
  }

  html << "</ul>\n</body>\n</html>";
  closedir(dir);
  return html.str();
}

char** CGI::setupEnvironment(const std::string& scriptPath) {
  std::map<std::string, std::string> env;

  // 基本的な環境変数を設定
  env["REQUEST_METHOD"] = _httpRequest.getMethod();
  env["SCRIPT_FILENAME"] = scriptPath;
  env["SCRIPT_NAME"] = _httpRequest.getURL();
  env["PATH_INFO"] = "";

  // クエリ文字列を取得
  size_t queryPos = _httpRequest.getURL().find('?');
  if (queryPos != std::string::npos) {
    env["QUERY_STRING"] = _httpRequest.getURL().substr(queryPos + 1);
  } else {
    env["QUERY_STRING"] = "";
  }

  // サーバー情報
  std::string host = _httpRequest.getHeader("Host");
  size_t colonPos = host.find(':');
  if (colonPos != std::string::npos) {
    env["SERVER_NAME"] = host.substr(0, colonPos);
    env["SERVER_PORT"] = host.substr(colonPos + 1);
  } else {
    env["SERVER_NAME"] = host;
    env["SERVER_PORT"] = "80";  // デフォルトポート
  }

  // コンテンツ情報
  if (!_httpRequest.getHeader("Content-Length").empty()) {
    env["CONTENT_LENGTH"] = _httpRequest.getHeader("Content-Length");
  }
  if (!_httpRequest.getHeader("Content-Type").empty()) {
    env["CONTENT_TYPE"] = _httpRequest.getHeader("Content-Type");
  }

  // HTTPヘッダー情報
  const std::map<std::string, std::string>& headers = _httpRequest.getHeaders();
  for (std::map<std::string, std::string>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    std::string name = "HTTP_" + it->first;
    // '-'を'_'に置換
    for (size_t i = 0; i < name.length(); i++) {
      if (name[i] == '-') name[i] = '_';
    }
    env[name] = it->second;
  }

  // 環境変数を文字列配列に変換
  char** envp = new char*[env.size() + 1];
  int i = 0;

  for (std::map<std::string, std::string>::const_iterator it = env.begin();
       it != env.end(); ++it) {
    std::string envStr = it->first + "=" + it->second;
    envp[i] = new char[envStr.size() + 1];
    std::strcpy(envp[i], envStr.c_str());
    i++;
  }

  envp[i] = NULL;  // 終端NULL

  return envp;
}

void CGI::cleanupEnv(char** env) const {
  if (env) {
    for (int i = 0; env[i] != NULL; i++) {
      delete[] env[i];
    }
    delete[] env;
  }
}

bool CGI::executeCGI(const std::string& scriptPath) {
  // スクリプトが存在するか確認
  std::ifstream scriptFile(scriptPath.c_str());
  if (!scriptFile.is_open()) {
    return false;
  }
  scriptFile.close();

  // 既存のCGI出力ファイルを削除
  unlink(CGI_PAGE);

  // 環境変数を設定
  char** envp = setupEnvironment(scriptPath);

  // 子プロセスを作成
  pid_t pid = fork();

  if (pid == -1) {
    // fork失敗
    cleanupEnv(envp);
    return false;
  } else if (pid == 0) {
    // 子プロセス

    // CGI出力をファイルにリダイレクト
    int fd = open(CGI_PAGE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      exit(1);
    }

    // 標準出力をファイルにリダイレクト
    dup2(fd, STDOUT_FILENO);
    close(fd);

    // POSTデータを標準入力から読めるようにする場合
    if (_httpRequest.getMethod() == "POST" && !_httpRequest.getBody().empty()) {
      // 一時ファイルにPOSTデータを書き込む
      std::string tmpFile = "/tmp/.cgi_post_data";
      std::ofstream postData(tmpFile.c_str(), std::ios::binary);
      postData << _httpRequest.getBody();
      postData.close();

      // 標準入力をPOSTデータから読む
      int inFd = open(tmpFile.c_str(), O_RDONLY);
      if (inFd != -1) {
        dup2(inFd, STDIN_FILENO);
        close(inFd);
      }
    }

    // 複数のPythonパスを試す
    const char* pythonPaths[] = {"/usr/bin/python3",
                                 "/usr/bin/python",
                                 "/usr/local/bin/python3",
                                 "/usr/local/bin/python",
                                 "/bin/python3",
                                 "/bin/python",
                                 "python3",
                                 "python",
                                 NULL};

    // 各Pythonパスを試す
    for (int i = 0; pythonPaths[i] != NULL; ++i) {
      execle(pythonPaths[i], pythonPaths[i], scriptPath.c_str(), NULL, envp);
    }

    // 最後の手段として環境の $PATH から探す
    execle("/usr/bin/env", "env", "python3", scriptPath.c_str(), NULL, envp);
    execle("/usr/bin/env", "env", "python", scriptPath.c_str(), NULL, envp);

    // すべて失敗した場合
    exit(1);
  } else {
    // 親プロセス
    cleanupEnv(envp);

    // タイムアウト設定
    int status;
    pid_t result;
    int timeout = CGI_TIMEOUT;

    // 子プロセスの終了を待つ (タイムアウト付き)
    std::time_t startTime = std::time(NULL);
    while ((std::time(NULL) - startTime) < timeout) {
      result = waitpid(pid, &status, WNOHANG);
      if (result == -1) {
        return false;  // エラー発生
      }
      if (result > 0) {
        // 子プロセス終了
        // スクリプトが正常に実行された場合はtrueを返す
        // 終了コードに関わらずCGI_PAGEファイルが存在するか確認
        std::ifstream testFile(CGI_PAGE);
        bool exists = testFile.good();
        testFile.close();
        return exists;
      }
      // usleep(100000) の代わりに select() を使用
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 100000;  // 100ミリ秒
      select(0, NULL, NULL, NULL, &tv);
    }

    // タイムアウトした場合、子プロセスを強制終了
    kill(pid, SIGTERM);
    // usleep(100000) の代わりに select() を使用
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 100ミリ秒
    select(0, NULL, NULL, NULL, &tv);

    kill(pid, SIGKILL);  // 確実に終了させる
    waitpid(pid, NULL, 0);
    return false;
  }
}

bool CGI::readCGIResponse() {
  // CGI実行結果ファイルが存在し、読み取り可能か確認
  std::ifstream cgiOutput(CGI_PAGE);
  if (!cgiOutput.is_open()) {
    return false;
  }

  // ファイルが空でないことを確認
  cgiOutput.seekg(0, std::ios::end);
  size_t size = cgiOutput.tellg();
  cgiOutput.close();

  return size > 0;
}

void CGI::handleRequest(HTTPResponse& httpResponse) {
  // 非Pythonスクリプトの場合は処理せず、次のハンドラーに委譲
  if (!isPythonScript(_httpRequest.getURL())) {
    httpResponse.setHttpStatusCode(
        0);  // ステータスコードを設定せず次のハンドラへ

    if (_nextHandler != NULL) {
      _nextHandler->handleRequest(httpResponse);
    }
    return;
  }

  // スクリプトパスを取得
  std::string scriptPath = getScriptPath();

  // URLがディレクトリを指している場合
  if (isDirectory(_httpRequest.getURL())) {
    // ディレクトリ内にPythonインデックスファイルがあるか確認
    bool hasIndexScript = false;

    // indexディレクティブをチェック
    const Directive* indexDirective = _rootDirective.findDirective(
        _httpRequest.getHeader("Host"), "location", _httpRequest.getURL());
    if (indexDirective != NULL) {
      std::string indexValue = indexDirective->getValue("index");
      if (!indexValue.empty() && isPythonScript(indexValue)) {
        // Python インデックスファイルを実行
        if (executeCGI(scriptPath) && readCGIResponse()) {
          // 成功: CGIが実行できたことを示すステータスコードを設定
          httpResponse.setHttpStatusCode(200);
          hasIndexScript = true;
        }
      }
    }

    // インデックスファイルがない場合はディレクトリリストを生成
    if (!hasIndexScript) {
      httpResponse.setHttpStatusCode(200);
      httpResponse.setHttpResponseBody(generateDirectoryListing(scriptPath));
    }
  }
  // 通常のPythonスクリプト実行
  else if (executeCGI(scriptPath) && readCGIResponse()) {
    // 成功: CGIが実行できたことを示すステータスコードを設定
    httpResponse.setHttpStatusCode(200);
  } else {
    // 失敗: 500 Internal Server Error
    httpResponse.setHttpStatusCode(500);
  }

  // ハンドラチェーンの次のハンドラに処理を移す
  if (_nextHandler != NULL) {
    _nextHandler->handleRequest(httpResponse);
  }
}
