#include "PrintResponse.hpp"

PrintResponse::PrintResponse() {}

PrintResponse::~PrintResponse() {}

void PrintResponse::print_response() 
{
    //PrintResponseの中身の
    send(fd, string , strlen , カスタムフラグ);
}



void send_http_response(int client_socket, const char *filename) {
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

    // ファイルのサイズを取得
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // ヘッダーを作成
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             file_size);

    // ヘッダーを送信
    send(client_socket, header, strlen(header), 0);

    // ファイルの内容を送信
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

//まだ完成していない
// #include "RunServer.hpp"
// int main(){
//    std::string home_path = getenv("HOME") ? getenv("HOME") : "";
//   std::cout  << "home_path: " << home_path << std::endl;
//   std::string file_path = home_path + "/Desktop/webserve/html/index.html";
//   std::cout << "file_path: " << file_path << std::endl;
//   send_http_response(get_poll_fds()[i].fd, file_path.c_str());
// }