#include "PrintResponse.hpp"

#include "HTTPHandleSuccess.hpp"

PrintResponse::PrintResponse() {}

PrintResponse::~PrintResponse() {}

void PrintResponse::send_header(int client_socket, FILE *file,
                                HTTPResponse response) {
  // ヘッダーを作成
  std::string header_str = response.connect_str(file);
  const char *header = header_str.c_str();

  // ヘッダー長を取得
  size_t header_len = header_str.length();

  // ヘッダーを送信（エラーチェック付き）
  ssize_t bytes_sent = send(client_socket, header, header_len, MSG_NOSIGNAL);
  if (bytes_sent == -1) {
    throw std::runtime_error("Failed to send header");
  }
  if (static_cast<size_t>(bytes_sent) != header_len) {
    throw std::runtime_error("Incomplete header send");
  }
}

//第３引数にメソッドの値を格納するクラスを作る。もしくはメソッドをテンプレート化する
void PrintResponse::send_http_response(int client_socket, const char *filename,
                                       HTTPResponse response) {
  std::cout << "send_http_response" << std::endl;
  FILE *file = fopen(filename, "r");
  if (!file) {
    // ファイルが開けなかった場合、404エラーを返す
    //本来ならエラーハンドルから渡されるものを使う
    const char *not_found_response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 46\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>";

    send(client_socket, not_found_response, strlen(not_found_response),
         MSG_NOSIGNAL);
    return;
  }
  send_header(client_socket, file, response);

  // ファイルの内容（ボディ）を送信
  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    send(client_socket, buffer, bytes_read, MSG_NOSIGNAL);
  }

  fclose(file);
}
