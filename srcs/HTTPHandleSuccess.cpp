#include "HTTPHandleSuccess.hpp"

#include "RunServer.hpp"

HTTPResponse::HTTPResponse() {
  _version = "HTTP/1.1";
  _status_code = 200;
  _status_message = "OK";
  _headers["Connection"] = "close";
  _body = "";
}

HTTPResponse::~HTTPResponse() {}

void HTTPResponse::setStatus(int code, const std::string& message) {
  _status_code = code;
  _status_message = message;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
  _headers[key] = value;
}

void HTTPResponse::setBody(const std::string& body) { _body = body; }

//ここにステータスコードとメッセージ、そしてheaderを一つの文字列にして返す関数を作る。ボディはファイルから読み込む
std::string HTTPResponse::connect_str(FILE* file) const {
  // ファイルのサイズを取得
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);
  std::string response = _version + " " + std::to_string(_status_code) + " " +
                         _status_message + "\n";
  for (const auto& header : _headers) {
    response += header.first + ": " + header.second + "\n";
  }
  response += "\n";
  response += _body;
  return response;
}

HTTPResponse HTTPResponse::ceateResponseData(RunServer& run_server, size_t i) {
  // HTTPレスポンスを作成
  HTTPResponse response;
  response.setStatus(
      200,
      "OK");  // methodから受け取る?methodから受け取るものはエラーのみ。正常なら200でいいので受け取らない？
  response.setHeader("Content-Type", "text/plain");  // methodから受け取る
  response.setBody("Hello from WebServ!");           // methodから受け取る

  return response;
}
