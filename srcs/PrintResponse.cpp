#include "PrintResponse.hpp"

PrintResponse::PrintResponse() {}

PrintResponse::~PrintResponse() {}

void PrintResponse::send_header(int client_socket, FILE *file) {
    // ファイルのサイズを取得
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    // ヘッダーを作成
    char header[BUFFER_SIZE];

    // snprintf()は、指定されたフォーマット文字列に従って文字列を生成し、
    // 生成された文字列をバッファに格納します。
    // 生成された文字列の長さは、バッファサイズ（バイト単位）によって制限されます。
    //メソッドから値を受け取る。これはモック
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             file_size);
    // ヘッダーを送信
    send(client_socket, header, strlen(header), 0);
}

void PrintResponse::send_http_response(int client_socket, const char *filename) {
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

        send(client_socket, not_found_response, strlen(not_found_response), 0);
        return;
    }
    send_header(client_socket, file);

    // ファイルの内容を送信
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}
