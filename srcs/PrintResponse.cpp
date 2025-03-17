#include "PrintResponse.hpp"

// #include "HTTPHandleSuccess.hpp"

PrintResponse::PrintResponse(int client_socket) {
  this->client_socket = client_socket;
}

PrintResponse::~PrintResponse() {}

//printresponseはすでにstatuscode, header, bodyを持っているので、それを使ってレスポンスを作成する
void PrintResponse::handleRequest(HTTPResponse &httpResponse) {
  // ステータスラインを送信
  // std::string status_line = "HTTP/1.1 " + std::to_string(httpResponse.getHttpStatusCode()) + " " + httpResponse.getHttpStatusLine() + "\r\n";
  // if (send(client_socket, status_line.c_str(), status_line.size(), MSG_NOSIGNAL) < 0) {
  if (send(client_socket, httpResponse.getHttpStatusLine().c_str(), httpResponse.getHttpStatusLine().size(), MSG_NOSIGNAL) < 0) {
    throw std::runtime_error("Failed to send status line");
  }

  // レスポンスヘッダを送信
  std::string response_header = httpResponse.getHttpResponseHeader();
  if (send(client_socket, response_header.c_str(), response_header.size(), MSG_NOSIGNAL) < 0) {
    throw std::runtime_error("Failed to send response header");
  }

 // レスポンスボディ送信
  std::string body_path = httpResponse.getHttpResponseBody();
  if (!body_path.empty()) {
    int fd = open(body_path.c_str(), O_RDONLY);
    if (fd < 0) {
      throw std::runtime_error("Failed to open response body file");
    }
    
    std::vector<std::string> chunks = asshuku(fd);
for (std::vector<std::string>::const_iterator it = chunks.begin(); it != chunks.end(); ++it) {
    if (send(client_socket, it->c_str(), it->size(), MSG_NOSIGNAL) < 0) {
        close(fd);
        throw std::runtime_error("Failed to send response body");
    }
}
    close(fd);
  }
}

std::vector<std::string> PrintResponse::asshuku(int fd) {
  std::vector<std::string> result;
  const size_t chunk_size = 1024;
  char buffer[chunk_size];
  ssize_t bytes_read;

  while ((bytes_read = read(fd, buffer, chunk_size)) > 0) {
    result.push_back(std::string(buffer, bytes_read));
  }

  if (bytes_read < 0) {
    throw std::runtime_error("Failed to read file");
  }

  return result;
}
