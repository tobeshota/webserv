#include "GenerateHTTPResponse.hpp"

std::string int2str(int nb) {
  std::stringstream ss;
  ss << nb;
  return ss.str();
}

std::string GenerateHTTPResponse::generateHttpStatusLine(
    const int status_code) {
  StatusCodes statusCodes;

  std::string httpStatusLine =
      std::string(_httpRequest.getVersion()) + " ";  //  http version
  httpStatusLine += int2str(status_code);            //  http status code
  httpStatusLine +=
      " " + statusCodes.getMessage(status_code) + "\n";  //  http status message
  return httpStatusLine;
}

std::string readFile(const std::string& filePath) {
  // ファイルを開く
  std::ifstream file(filePath.c_str());

  // ファイルが正常に開けなかった場合
  if (!file.is_open()) {
    return "";
  }

  // ストリームバッファを使って効率的に読み込む
  std::stringstream buffer;
  buffer << file.rdbuf();

  // ファイルは自動的に閉じられる（RAIIパターン）
  return buffer.str();
}

std::string GenerateHTTPResponse::generateHttpResponseHeader(
    const std::string& httpResponseBody) {
  std::string httpResponseHeader = "Server: webserv\n";
  // httpResponseHeader += "Date: " + getCurrentTimeInGMTFormat() + "\n";
  httpResponseHeader += "Content-Type: text/html\n";
  httpResponseHeader +=
      "Content-Length: " + int2str(httpResponseBody.size()) + "\n";
  httpResponseHeader += "Connection: close\n";
  return httpResponseHeader;
}

bool isDirectory(const std::string& filePath) {
  // 文字列が空でないかつ最後の文字が '/' であるかを確認
  if (!filePath.empty() && filePath[filePath.length() - 1] == '/') {
    return true;
  }
  return false;
}

std::string GenerateHTTPResponse::getPathForHttpResponseBody(
    const int status_code) {
  // エラーステータスコード（2xx以外）の場合
  if (status_code / 100 != 2) {
    std::string errorPageValue, rootValue;

    // ステータスコードに対応するerror_pageディレクティブを探す
    const Directive* errorPageDirective = _rootDirective.findDirective(
        _httpRequest.getHeader("Host"), "error_page");
    if (errorPageDirective != NULL) {
      errorPageValue = errorPageDirective->getValue(int2str(status_code));
    }

    // ホストディレクティブからrootの値を取得
    const Directive* hostDirective =
        _rootDirective.findDirective(_httpRequest.getHeader("Host"));
    if (hostDirective != NULL) {
      rootValue = hostDirective->getValue("root");
    }

    // カスタムエラーページが設定されていればそのパスを返す
    if (!errorPageValue.empty()) {
      return rootValue + errorPageValue;
    }
    // 設定がなければデフォルトのエラーページを返す
    return DEFAULT_ERROR_PAGE;
  }

  // 成功ステータス（2xx）の場合
  std::string requestedURL = _httpRequest.getURL();
  std::string rootValue;

  // ホストディレクティブからrootの値を取得
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getHeader("Host"));
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  // URLがディレクトリの場合
  if (isDirectory(requestedURL)) {
    // インデックスファイルを探す
    const Directive* indexDirective = _rootDirective.findDirective(
        _httpRequest.getHeader("Host"), "location", requestedURL);
    if (indexDirective != NULL) {
      std::string indexValue = indexDirective->getValue("index");
      if (!indexValue.empty()) {
        return rootValue + requestedURL + indexValue;
      }
    }
    // インデックスディレクティブがなければデフォルトのindex.htmlを使用
    return rootValue + requestedURL + "index.html";
  }

  // リクエストされたリソースのフルパスを返す
  return rootValue + requestedURL;
}

// 指定された文字列が任意の文字列で終わるかを調べる関数
static bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string GenerateHTTPResponse::getDirectiveValue(std::string directiveKey) {
  std::string directiveValue;
  // 指定のホスト内の指定のロケーション内で指定ディレクティブdirectiveKeyの値があれば取得する
  std::string requestedURL = _httpRequest.getURL();
  if (isDirectory(requestedURL)) {
    const Directive* locationDirective = _rootDirective.findDirective(
        _httpRequest.getHeader("Host"), "location", requestedURL);
    if (locationDirective != NULL) {
      directiveValue = locationDirective->getValue(directiveKey);
      if (!directiveValue.empty()) return directiveValue;
    }
  }

  // 指定のホスト内で指定ディレクティブdirectiveKeyの値があれば取得する
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getHeader("Host"));
  if (hostDirective != NULL) {
    directiveValue = hostDirective->getValue(directiveKey);
    if (!directiveValue.empty()) return directiveValue;
  }

  return "";
}

std::string GenerateHTTPResponse::generateHttpResponseBody(
    const int status_code, bool& pageFound) {
  // DELETEメソッドがコールされた場合，HTTPレスポンスボディを空にする
  if (_httpRequest.getMethod() == "DELETE") return "";

  std::string httpResponseBody;

  // HTTPレスポンスがCGIの実行結果であるか
  if (endsWith(_httpRequest.getURL(), ".py") ||
      endsWith(_httpRequest.getURL(), ".sh")) {
    httpResponseBody = readFile(CGI_PAGE);
  }  // HTTPリダイレクトすべきか
  else if (getDirectiveValue("return") != "" &&
           getDirectiveValue("root") != "") {
    std::string redirectURL =
        getDirectiveValue("root") + "/" + getDirectiveValue("return") + ".html";
    httpResponseBody = readFile(redirectURL);
  }
  // ディレクトリリスニングすべきか
  else if (status_code == 200 && getDirectiveValue("autoindex") == "on" &&
           getDirectiveValue("root") != "") {
    ListenDirectory listenDirectory(getDirectiveValue("root"));
    HTTPResponse response;
    listenDirectory.handleRequest(response);
    httpResponseBody = response.getHttpResponseBody();
  } else {
    httpResponseBody = readFile(getPathForHttpResponseBody(status_code));
  }

  if (httpResponseBody.empty()) {
    httpResponseBody = readFile(DEFAULT_ERROR_PAGE);
    pageFound = false;
  }
  return httpResponseBody;
}

GenerateHTTPResponse::GenerateHTTPResponse(Directive rootDirective,
                                           HTTPRequest httpRequest)
    : _rootDirective(rootDirective), _httpRequest(httpRequest) {}

void GenerateHTTPResponse::handleRequest(HTTPResponse& httpResponse) {
  bool pageFound = true;

  httpResponse.setHttpResponseBody(this->generateHttpResponseBody(
      httpResponse.getHttpStatusCode(), pageFound));
  if (pageFound == false) {
    httpResponse.setHttpStatusCode(404);
  }
  httpResponse.setHttpStatusLine(
      this->generateHttpStatusLine(httpResponse.getHttpStatusCode()));
  httpResponse.setHttpResponseHeader(
      this->generateHttpResponseHeader(httpResponse.getHttpResponseBody()));

  if (_nextHandler != NULL) _nextHandler->handleRequest(httpResponse);
}
