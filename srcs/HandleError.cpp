#include "HandleError.hpp"

std::string int2str(int nb) {
    std::stringstream ss;
    ss << nb;
    return ss.str();
}

std::string HandleError::generateHttpStatusLine(const int status_code) {
  StatusCodes statusCodes;

  std::string httpStatusLine = std::string(_httpRequest.getVersion()) + " ";  //  http version
  httpStatusLine += int2str(status_code);  //  http status code
  httpStatusLine += " " + statusCodes.getMessage(status_code) + "\n";  //  http status message
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

// DateのHTTPレスポンスヘッダは省略する．必要になったら設定する．
// #include <ctime>  // 時刻の取得や操作を行うためのヘッダ。time(), gmtime(), struct tmなどの機能を提供。
// #include <iomanip>  // 入力/出力のフォーマット設定を行うためのヘッダ。std::setfill('0')やstd::setw(2)など、表示フォーマットを制御するために使用。
// std::string getCurrentTimeInGMTFormat() {
//   // 現在の時刻を取得
//   time_t rawtime;
//   struct tm* timeinfo;
//   time(&rawtime);
//   timeinfo = gmtime(&rawtime);  // GMT(UTC)に変換

//   // 曜日の配列
//   const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//   // 月の配列
//   const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
//                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

//   // 出力用の文字列ストリーム
//   std::stringstream result;

//   // 現在時刻を指定の形式でストリームに追加
//   result << weekdays[timeinfo->tm_wday] << ", " << timeinfo->tm_mday << " "
//          << months[timeinfo->tm_mon] << " " << (1900 + timeinfo->tm_year) << " "
//          << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":"
//          << std::setfill('0') << std::setw(2) << timeinfo->tm_min << ":"
//          << std::setfill('0') << std::setw(2) << timeinfo->tm_sec << " GMT";

//   // 結果を返す
//   return result.str();
// }

std::string HandleError::generateHttpResponseHeader(
    const std::string& httpResponseBody) {
  std::string httpResponseHeader = "Server: webserv\n";
  // httpResponseHeader += "Date: " + getCurrentTimeInGMTFormat() + "\n";
  httpResponseHeader += "Content-Type: text/html\n";
  httpResponseHeader += "Content-Length: " + int2str(httpResponseBody.size()) + "\n";
  httpResponseHeader += "Connection: close\n";
  return httpResponseHeader;
}

// webserv.confのディレクティブerror_page, index指定のファイル
// error_page 404 /custom_404.html; のように設定できる．
std::string HandleError::generateHttpResponseBody(const int status_code) {
  std::string httpResponseBody, errorPageValue, rootValue;

  const Directive *errorPageDirective = _rootDirective.findDirective(_httpRequest.getHeader("Host"), "error_page");
  if (errorPageDirective == NULL)
    return readFile(DEFAULT_ERROR_PAGE);
  errorPageValue = errorPageDirective->getValue(int2str(status_code));

  const Directive* hostDirective = _rootDirective.findDirective(_httpRequest.getHeader("Host"));
  if (hostDirective != NULL) {
    rootValue = hostDirective->getValue("root");
  }

  const std::string& errorPagePath = rootValue + errorPageValue;

  httpResponseBody = readFile(errorPagePath);
  if (httpResponseBody.empty()) {
    httpResponseBody = readFile(DEFAULT_ERROR_PAGE);
  }

  return httpResponseBody;
}

HandleError::HandleError(Directive rootDirective, HTTPRequest httpRequest)
    : _rootDirective(rootDirective),
    _httpRequest(httpRequest) {}

void HandleError::handleRequest(HTTPResponse& httpResponse) {
  httpResponse.setHttpStatusLine( this->generateHttpStatusLine(httpResponse.getHttpStatusCode()));
  httpResponse.setHttpResponseBody(this->generateHttpResponseBody(httpResponse.getHttpStatusCode()));
  httpResponse.setHttpResponseHeader( this->generateHttpResponseHeader(httpResponse.getHttpResponseBody()));

  if (_nextHandler != NULL)
    _nextHandler->handleRequest(httpResponse);
}
