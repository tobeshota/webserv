#include "webserv.hpp"

// int main(int argc, char **argv) { return webserv(argc, argv); }

//this is OSInit's main
int main() {
  OSInit os;
  os.initServer();
  return 0;
}








// int main() {
//     int sock = 0;
//     struct sockaddr_in serv_addr;
//     char buffer[1024] = {0};

//     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//         std::cerr << "Socket creation error" << std::endl;
//         return -1;
//     }

//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_port = htons(8080);

//     // サーバーのIPアドレスを設定
//     if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
//         std::cerr << "Invalid address/ Address not supported" << std::endl;
//         return -1;
//     }

//     if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
//         std::cerr << "Connection Failed" << std::endl;
//         return -1;
//     }

//     const char *hello = "Hello from client";
//     send(sock, hello, strlen(hello), 0);
//     std::cout << "Hello message sent" << std::endl;

//     int valread = read(sock, buffer, 1024);
//     std::cout << "Received: " << buffer << std::endl;
//     std::cout << "valread: " << valread << std::endl;

//     close(sock);
//     return 0;
// }
