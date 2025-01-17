#include "OSInit.hpp"

OSInit::OSInit() {
}

OSInit::~OSInit() {
}

// サーバーを構築する
void OSInit::initServer() {

    // サーバーの構築
    server_data.set_address_data();
    server_data.set_server_fd();
    server_data.server_bind();
    server_data.server_listen();

    std::cout << "Startup complete!, Start-up completed!" << std::endl;
    std::cout << " The number of people who can connect to this server remaining is " << MAX_CONNECTION  << std::endl;
    server_data.server_accept();

    while (true) 
    {
        int poll_count = poll(server_data.get_poll_fds().data(), server_data.get_poll_fds().size(), -1);
        if (poll_count == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < server_data.get_poll_fds().size(); ++i)
        {
            if (server_data.get_poll_fds()[i].revents & POLLIN) {
                if (server_data.get_poll_fds()[i].fd == server_data.get_server_fd())
                {
                    // 新しい接続を受け入れる
                    int new_socket = accept(server_data.get_server_fd(), nullptr, nullptr);
                    if (new_socket == -1)
                    {
                        perror("accept");
                        continue;
                    }
                    std::cout << "New connection accepted" << std::endl;

                    // 新しいクライアントのpollfdを追加
                    pollfd client_fd_poll;
                    client_fd_poll.fd = new_socket;
                    client_fd_poll.events = POLLIN;
                    server_data.get_poll_fds().push_back(client_fd_poll);
                } 
                else 
                {
                    // クライアントからのデータを受信
                    char buffer[1024];
                    ssize_t bytes_read = read(server_data.get_poll_fds()[i].fd, buffer, sizeof(buffer));
                    if (bytes_read == -1)
                    {
                        perror("read");
                        close(server_data.get_poll_fds()[i].fd);
                        server_data.get_poll_fds().erase(server_data.get_poll_fds().begin() + i);
                        --i;
                    } else if (bytes_read == 0)
                    {
                        // クライアントが切断
                        std::cout << "Client disconnected" << std::endl;
                        close(server_data.get_poll_fds()[i].fd);
                        server_data.get_poll_fds().erase(server_data.get_poll_fds().begin() + i);
                        --i;
                    } else {
                        // 受信データを処理
                        buffer[bytes_read] = '\0';
                        std::cout << "Received: " << buffer << std::endl;
                        // 受信データをクライアントに送信（エコー）
                    ssize_t bytes_sent = write(server_data.get_poll_fds()[i].fd, buffer, bytes_read);
                    if (bytes_sent == -1) {
                        perror("write");
                        close(server_data.get_poll_fds()[i].fd);
                        server_data.get_poll_fds().erase(server_data.get_poll_fds().begin() + i);
                        --i;
                    }
                    }
                }
            }
        }
    }
    close(server_data.get_server_fd());
  // webserv.conf指定のポート番号でのリッスンを受け付ける
  std::cout << "initServer" << std::endl;
}
