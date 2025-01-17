#include "OSInit.hpp"

OSInit::OSInit() {
}

OSInit::~OSInit() {
}

void OSInit::set_serverpoll_data() {
    // poll_fdsの初期設定
    pollfd server_fd_poll;
    server_fd_poll.fd = server_data.get_server_fd();
    server_fd_poll.events = POLLIN;
    poll_fds.push_back(server_fd_poll);
}

int OSInit::check_func(int func, std::string error_message) {
    if (func == -1) {
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }
    return func;
}

void OSInit::run() 
{
    while(true)
    {
        int poll_count = check_func(poll(poll_fds.data(), poll_fds.size(), -1), "poll");
        //unnuse
        std::cout << poll_count << std::endl;
        process_poll_events();
    }
}

void OSInit::handle_new_connection(int server_fd)
{
    int new_socket = accept(server_fd, nullptr, nullptr);
    if (new_socket == -1) {
        perror("accept");
        return;
    }
    std::cout << "New connection accepted" << std::endl;

    pollfd client_fd_poll;
    client_fd_poll.fd = new_socket;
    client_fd_poll.events = POLLIN;
    poll_data.get_poll_fds().push_back(client_fd_poll);
}


 void OSInit::handle_client_data(size_t i) {
        char buffer[1024];
        ssize_t bytes_read = read(poll_data.get_poll_fds()[i].fd, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("read");
            close(poll_data.get_poll_fds()[i].fd);
            poll_data.get_poll_fds().erase(poll_data.get_poll_fds().begin() + i);
            --i;
        } else if (bytes_read == 0) {
            std::cout << "Client disconnected" << std::endl;
            close(poll_data.get_poll_fds()[i].fd);
            poll_data.get_poll_fds().erase(poll_data.get_poll_fds().begin() + i);
            --i;
        } else {
            buffer[bytes_read] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            ssize_t bytes_sent = write(poll_data.get_poll_fds()[i].fd, buffer, bytes_read);
            if (bytes_sent == -1) {
                perror("write");
                close(poll_data.get_poll_fds()[i].fd);
                poll_data.get_poll_fds().erase(poll_data.get_poll_fds().begin() + i);
                --i;
            }
        }
    }


void OSInit::process_poll_events()
{
    for (size_t i = 0; i < poll_data.get_poll_fds().size(); ++i) {
        if (poll_data.get_poll_fds()[i].revents & POLLIN) {
            if (poll_data.get_poll_fds()[i].fd == server_data.get_server_fd()) {
                handle_new_connection(server_data.get_server_fd());
            } else {
                handle_client_data(i);
            }
        }
    }
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
    run();
    close(server_data.get_server_fd());
  // webserv.conf指定のポート番号でのリッスンを受け付ける
  std::cout << "initServer" << std::endl;
}
