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
      " " + statusCodes.getMessage(status_code) + "\r\n";  //  http status message
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

// ファイル拡張子を取得するヘルパー関数
std::string GenerateHTTPResponse::getFileExtension(const std::string& filePath) {
  size_t dotPos = filePath.find_last_of('.');
  if (dotPos != std::string::npos && dotPos < filePath.length() - 1) {
    return filePath.substr(dotPos);
  }
  return "";
}

// MIMEタイプを取得する関数
std::string GenerateHTTPResponse::getMimeType(const std::string& filePath) {
  std::string extension = getFileExtension(filePath);
  if (extension.empty()) {
    return "text/html"; // デフォルト
  }

  // 設定ファイルからContent-Typeを探す
  std::string wildcardPattern = "*" + extension;
  const Directive* locationDirective = _rootDirective.findDirective(
      _httpRequest.getServerName(), "location", wildcardPattern);

  if (locationDirective != NULL) {
    std::string contentType = locationDirective->getValue("Content-Type");
    if (!contentType.empty()) {
      // セミコロンで終わっている場合は除去
      if (!contentType.empty() && contentType[contentType.length() - 1] == ';') {
        contentType = contentType.substr(0, contentType.length() - 1);
      }
      return contentType;
    }
  }

  // 設定がない場合はデフォルトのMIMEタイプを返す
  if (extension == ".html" || extension == ".htm") return "text/html";
  if (extension == ".js") return "text/javascript";
  if (extension == ".css") return "text/css";
  if (extension == ".json") return "application/json";
  if (extension == ".png") return "image/png";
  if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
  if (extension == ".gif") return "image/gif";
  if (extension == ".svg") return "image/svg+xml";
  if (extension == ".pdf") return "application/pdf";
  if (extension == ".xml") return "application/xml";
  if (extension == ".mp3") return "audio/mpeg";
  if (extension == ".wav") return "audio/wav";
  if (extension == ".ogg") return "audio/ogg";
  if (extension == ".mp4") return "video/mp4";
  if (extension == ".webm") return "video/webm";
  if (extension == ".ico") return "image/x-icon";
  if (extension == ".txt") return "text/plain";
  if (extension == ".woff") return "font/woff";
  if (extension == ".woff2") return "font/woff2";
  if (extension == ".ttf") return "font/ttf";
  if (extension == ".eot") return "application/vnd.ms-fontobject";

  return "text/html"; // デフォルト
}

std::string GenerateHTTPResponse::generateHttpResponseHeader(
    const std::string& httpResponseBody) {
  std::string httpResponseHeader = "Server: webserv\r\n";

  // ファイルパスを取得してMIMEタイプを設定
  std::string filePath;
  if (_httpRequest.getMethod() != "DELETE") {
    filePath = getSuccessPathForHttpResponseBody();
  }
  std::string mimeType = getMimeType(filePath);

  httpResponseHeader += "Content-Type: " + mimeType + "\r\n";
  httpResponseHeader +=
      "Content-Length: " + int2str(httpResponseBody.size()) + "\r\n";
  httpResponseHeader += "Connection: close\r\n";
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
        requestedURL[requestedURL.length() - 1] == '/' ? "index.html" : "/index.html";
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
    // デフォルトエラーページも読めない場合は最低限のHTMLを生成
    if (httpResponseBody.empty()) {
      httpResponseBody = "<html><body><h1>Error " + int2str(status_code) + "</h1></body></html>";
    }
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
