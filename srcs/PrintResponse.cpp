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

// void mock(HTTPResponse &httpResponse) {
//   httpResponse.setHttpStatusLine("HTTP/1.1 200 OK\r\n");
//   httpResponse.setHttpResponseHeader("Content-Type: text/html\r\n\r\n");
//   httpResponse.setHttpResponseBody(
//       "/Users/yoshimurahiro/Desktop/wevserv_FR47/html/index.html");
// }

void PrintResponse::handleRequest(HTTPResponse &httpResponse) {
  // mock(httpResponse);
  // ステータスラインを送信
  if (send(client_socket, httpResponse.getHttpStatusLine().c_str(),
           httpResponse.getHttpStatusLine().size(), MSG_NOSIGNAL) < 0) {
    throw std::runtime_error("Failed to send status line");
  }

  // レスポンスヘッダを送信
  std::string response_header = httpResponse.getHttpResponseHeader();
  if (send(client_socket, response_header.c_str(), response_header.size(),
           MSG_NOSIGNAL) < 0) {
    throw std::runtime_error("Failed to send response header");
  }

  // レスポンスボディ送信
  std::string body_path = httpResponse.getHttpResponseBody();
  if (!body_path.empty()) {
    int fd = open(body_path.c_str(), O_RDONLY);
    if (fd < 0) {
      throw std::runtime_error("Failed to open response body file");
    }
    sendFileContent(fd, client_socket);
    close(fd);
  }
}
