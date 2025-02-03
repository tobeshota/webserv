#include <gtest/gtest.h>
#include <vector>
#include "RunServer.hpp"
#include "ServerData.hpp"

// pollfdの追加テスト
TEST(RunServerTest, AddPollFdTest) {
    RunServer server;
    pollfd test_fd;
    test_fd.fd = 3;
    test_fd.events = POLLIN;
    server.add_poll_fd(test_fd);
    
    EXPECT_EQ(server.get_poll_fds().size(), 1);
    EXPECT_EQ(server.get_poll_fds()[0].fd, 3);
}

// クライアント接続処理テスト（モック）
TEST(RunServerTest, HandleNewConnectionTest) {
    ServerData server;
    server.set_server_fd();
    server.set_address_data();
    server.server_bind();
    server.server_listen();

    RunServer run_server;
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_fd, -1);

    struct sockaddr_in serv_addr = server.get_address();
    EXPECT_EQ(connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), 0);

    run_server.handle_new_connection(server.get_server_fd());

    EXPECT_EQ(run_server.get_poll_fds().size(), 1);
    
    close(client_fd);
}

// クライアントデータ処理テスト
TEST(RunServerTest, HandleClientDataTest) {
    RunServer run_server;

    pollfd client_fd_poll;
    int pipe_fds[2];
    pipe(pipe_fds);

    client_fd_poll.fd = pipe_fds[0];
    client_fd_poll.events = POLLIN;
    run_server.add_poll_fd(client_fd_poll);

    const char* msg = "Hello";
    write(pipe_fds[1], msg, 5);
    
    run_server.handle_client_data(0);
    
    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

// イベント処理テスト
TEST(RunServerTest, ProcessPollEventsTest) {
    ServerData server;
    server.set_server_fd();
    server.set_address_data();
    server.server_bind();
    server.server_listen();

    RunServer run_server;
    run_server.add_poll_fd({server.get_server_fd(), POLLIN, 0});

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    connect(client_fd, (struct sockaddr*)&server.get_address(), sizeof(server.get_address()));

    run_server.process_poll_events(server);

    EXPECT_EQ(run_server.get_poll_fds().size(), 1);
    
    close(client_fd);
}
