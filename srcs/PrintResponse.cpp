#include "PrintResponse.hpp"

void sendFileContent(int fd, int client_socket) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
    if (send(client_socket, buffer, bytes_read, MSG_NOSIGNAL) < 0) {
      throw std::runtime_error("Failed to send file content");
    }
  }

  if (bytes_read < 0) {
    throw std::runtime_error("Failed to read file");
  }
}

PrintResponse::PrintResponse(int client_socket) {
  this->client_socket = client_socket;
}

PrintResponse::~PrintResponse() {}

void sendABit(const int client_socket, const std::string& str) {
  // 文字列の長さが10文字未満でも1回だけ送信するようにする
  size_t start = 0;
  size_t len = str.length();
  while (start < len) {
    // 10文字分を切り取って送信
    if (send(client_socket, str.substr(start, 10).c_str(),
             str.substr(start, 10).size(), MSG_NOSIGNAL) < 0) {
      throw std::runtime_error("Failed to send response body");
    }
    start += 10;
  }
}

void PrintResponse::handleRequest(HTTPResponse& httpResponse) {
  // mock(httpResponse);
  // ステータスラインを送信
  if (send(client_socket, httpResponse.getHttpStatusLine().c_str(),
           httpResponse.getHttpStatusLine().size(), MSG_NOSIGNAL) < 0) {
    throw std::runtime_error("Failed to send status line");
  }

  // レスポンスヘッダを送信
  std::string response_header = httpResponse.getHttpResponseHeader();
  response_header += "\r\n";  //  理由：ヘッダーとボディを分けるため
  if (send(client_socket, response_header.c_str(), response_header.size(),
           MSG_NOSIGNAL) < 0) {
    throw std::runtime_error("Failed to send response header");
  }

  // レスポンスボディ送信
  sendABit(client_socket, httpResponse.getHttpResponseBody());
}
