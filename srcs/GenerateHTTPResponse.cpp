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
  httpResponseHeader += "Content-Type: text/html\n";
  httpResponseHeader +=
      "Content-Length: " + int2str(httpResponseBody.size()) + "\n";
  httpResponseHeader += "Connection: close\n";
  return httpResponseHeader;
}

// filePathがサーバー上に存在するディレクトリかどうかを調べる
bool isDirectory(const std::string& filePath) {
  struct stat st;
  if (stat(filePath.c_str(), &st) != 0) {
    return false;
  }
  return S_ISDIR(st.st_mode);
}

// エラーステータスコード（2xxまたは301）の場合のファイルパスを取得する
std::string GenerateHTTPResponse::getErrorPathForHttpResponseBody(
    const int status_code) {
  std::string errorPageValue, rootValue;

  // ステータスコードに対応するerror_pageディレクティブを探す
  const Directive* errorPageDirective =
      _rootDirective.findDirective(_httpRequest.getServerName(), "error_page");
  if (errorPageDirective != NULL) {
    errorPageValue = errorPageDirective->getValue(int2str(status_code));
  }

  // ホストディレクティブからrootの値を取得
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getServerName());
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  // カスタムエラーページが設定されており、かつ空でなければそのパスを返す
  if (!errorPageValue.empty() &&
      !readFile(rootValue + errorPageValue).empty()) {
    return rootValue + errorPageValue;
  }
  // 設定がなければデフォルトのエラーページを返す
  return DEFAULT_ERROR_PAGE;
}

// 成功ステータス（2xxまたは301）の場合のファイルパスを取得する
std::string GenerateHTTPResponse::getSuccessPathForHttpResponseBody() {
  std::string requestedURL = _httpRequest.getURL();
  std::string rootValue = "";

  // ホストディレクティブからrootの値を取得
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getServerName());
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  // URLがディレクトリの場合
  if (isDirectory(rootValue + requestedURL)) {
    // インデックスファイルを探す
    const Directive* indexDirective = _rootDirective.findDirective(
        _httpRequest.getServerName(), "location", requestedURL);
    if (indexDirective != NULL) {
      std::string indexValue = indexDirective->getValue("index");
      if (!indexValue.empty()) {
        return rootValue + requestedURL + indexValue;
      }
    }
    // インデックスディレクティブがなければデフォルトのindex.htmlを使用
    std::string defaultIndexFileName =
        requestedURL.end()[-1] == '/' ? "index.html" : "/index.html";
    return rootValue + requestedURL + defaultIndexFileName;
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
  return getDirectiveValues(directiveKey)[0];
}

std::vector<std::string> GenerateHTTPResponse::getDirectiveValues(
    std::string directiveKey) {
  std::vector<std::string> directiveValues;

  // ホストディレクティブからrootの値を取得
  std::string rootValue = "";
  const Directive* hostDirective =
      _rootDirective.findDirective(_httpRequest.getServerName());
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  // 指定のホスト内の指定のロケーション内で指定ディレクティブdirectiveKeyの値があれば取得する
  std::string requestedURL = _httpRequest.getURL();
  if (isDirectory(rootValue + requestedURL)) {
    const Directive* locationDirective = _rootDirective.findDirective(
        _httpRequest.getServerName(), "location", requestedURL);
    if (locationDirective != NULL) {
      directiveValues = locationDirective->getValues(directiveKey);
      if (!directiveValues.empty()) return directiveValues;
    }
  }

  // 「指定のホスト内直下にある」かどうかを調べる．
  if (hostDirective != NULL) {
    directiveValues = hostDirective->getValues(directiveKey, false);
    if (!directiveValues.empty()) {
      return directiveValues;
    }
  }

  // 何もないものを返す
  std::vector<std::string> emptyVector;
  emptyVector.push_back("");  // 空文字列を追加
  return emptyVector;
}

std::string GenerateHTTPResponse::generateHttpResponseBody(
    const int status_code) {
  // DELETEメソッドがコールされた場合，HTTPレスポンスボディを空にする
  if (_httpRequest.getMethod() == "DELETE") return "";

  std::string httpResponseBody;

  // CGIは実行されたか（2xx番でないと実行されていない）
  if (status_code / 100 == 2 && (endsWith(_httpRequest.getURL(), ".py") ||
                                 endsWith(_httpRequest.getURL(), ".sh"))) {
    httpResponseBody = readFile(CGI_PAGE);
  }
  // ディレクトリリスニングすべきか
  else if (status_code != 400 && getDirectiveValue("autoindex") == "on" &&
           getDirectiveValue("root") != "" &&
           isDirectory(getDirectiveValue("root") + _httpRequest.getURL())) {
    ListenDirectory listenDirectory(getDirectiveValue("root") +
                                    _httpRequest.getURL());
    HTTPResponse response;
    listenDirectory.handleRequest(response);
    httpResponseBody = response.getHttpResponseBody();
  }
  // 成功しているか
  else if (status_code / 100 == 2 || status_code == 301) {
    httpResponseBody = readFile(getSuccessPathForHttpResponseBody());
  }

  // 読み取ったファイルが空の場合
  if (httpResponseBody.empty()) {
    httpResponseBody = readFile(getErrorPathForHttpResponseBody(status_code));
  }
  return httpResponseBody;
}

GenerateHTTPResponse::GenerateHTTPResponse(Directive rootDirective,
                                           HTTPRequest httpRequest)
    : _rootDirective(rootDirective), _httpRequest(httpRequest) {}

void GenerateHTTPResponse::handleRequest(HTTPResponse& httpResponse) {
  // リダイレクトが指定されている場合HttpStatusCodeを301に設定する
  if (getDirectiveValue("return") != "") {
    httpResponse.setHttpStatusCode(301);
  }

  httpResponse.setHttpResponseBody(
      this->generateHttpResponseBody(httpResponse.getHttpStatusCode()));
  httpResponse.setHttpStatusLine(
      this->generateHttpStatusLine(httpResponse.getHttpStatusCode()));
  httpResponse.setHttpResponseHeader(
      this->generateHttpResponseHeader(httpResponse.getHttpResponseBody()));

  if (_nextHandler != NULL) _nextHandler->handleRequest(httpResponse);
}
