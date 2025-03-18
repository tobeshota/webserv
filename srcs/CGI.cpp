#include "CGI.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <map>
#include <iostream>

CGI::CGI(Directive rootDirective, HTTPRequest httpRequest)
    : _rootDirective(rootDirective), _httpRequest(httpRequest) {}

CGI::~CGI() {}

bool CGI::isPythonScript(const std::string& url) const {
  // URLの拡張子が.pyかどうかをチェック
  size_t dotPos = url.find_last_of('.');
  if (dotPos != std::string::npos) {
    std::string extension = url.substr(dotPos);
    return extension == ".py";
  }
  return false;
}

std::string CGI::getScriptPath() const {
  std::string url = _httpRequest.getURL();
  std::string rootValue = "/tmp/webserv/www";  // デフォルトのroot値

  // クエリパラメータを削除
  size_t queryPos = url.find('?');
  if (queryPos != std::string::npos) {
    url = url.substr(0, queryPos);
  }

  // ホストディレクティブからrootの値を取得
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getHeader("Host"));
  if (hostDirective != NULL && !hostDirective->getValue("root").empty()) {
    rootValue = hostDirective->getValue("root");
  }

  // rootの末尾に/がない場合は追加
  if (!rootValue.empty() && rootValue[rootValue.length() - 1] != '/') {
    rootValue += '/';
  }

  // URLが/で始まっている場合は削除（ルートと連結するため）
  if (!url.empty() && url[0] == '/') {
    url = url.substr(1);
  }

  // スクリプトの完全なパスを返す
  return rootValue + url;
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
    env["SERVER_PORT"] = "80"; // デフォルトポート
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
    const char* pythonPaths[] = {
      "/usr/bin/python3",
      "/usr/bin/python",
      "/usr/local/bin/python3",
      "/usr/local/bin/python",
      "/bin/python3",
      "/bin/python",
      "python3",
      "python",
      NULL
    };

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
    time_t startTime = time(NULL);
    while ((time(NULL) - startTime) < timeout) {
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
      usleep(100000);  // 100ミリ秒待機
    }

    // タイムアウトした場合、子プロセスを強制終了
    kill(pid, SIGTERM);
    usleep(100000);
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
  // 最初にステータスコードをリセット（非Python処理時にランダムな値が入るのを防ぐ）
  if (!isPythonScript(_httpRequest.getURL())) {
    // 非Pythonスクリプトの場合は処理せず、次のハンドラーに委譲
    httpResponse.setHttpStatusCode(0);
    
    if (_nextHandler != NULL) {
      _nextHandler->handleRequest(httpResponse);
    }
    return;
  }

  // Pythonスクリプトの処理
  std::string scriptPath = getScriptPath();
  
  // CGIスクリプトを実行
  if (executeCGI(scriptPath) && readCGIResponse()) {
    // 成功: CGIの出力をレスポンスに設定
    httpResponse.setHttpStatusCode(200);
    
    // CGI出力をレスポンスボディに設定
    std::ifstream cgiFile(CGI_PAGE);
    std::stringstream buffer;
    buffer << cgiFile.rdbuf();
    httpResponse.setHttpResponseBody(buffer.str());
    cgiFile.close();
  } else {
    // 失敗: 500 Internal Server Error
    httpResponse.setHttpStatusCode(500);
  }

  // ハンドラチェーンの次のハンドラに処理を移す
  if (_nextHandler != NULL) {
    _nextHandler->handleRequest(httpResponse);
  }
}
