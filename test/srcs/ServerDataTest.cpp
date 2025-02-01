#include <gtest/gtest.h>
#include "ServerData.hpp"

class ServerDataTest : public ::testing::Test {
protected:
    ServerData serverData;
};

TEST_F(ServerDataTest, ConstructorInitializesCorrectly) {
    EXPECT_EQ(serverData.get_server_fd(), 0);
    EXPECT_EQ(serverData.get_new_socket(), 0);
    EXPECT_EQ(serverData.get_addrlen(), sizeof(struct sockaddr_in));
}

TEST_F(ServerDataTest, SetAddressData) {
    serverData.set_address_data();
    struct sockaddr_in address = serverData.get_address();
    
    EXPECT_EQ(address.sin_family, AF_INET);
    EXPECT_EQ(address.sin_addr.s_addr, INADDR_ANY);
    EXPECT_EQ(address.sin_port, htons(PORT)); // PORTは適宜定義
}

TEST_F(ServerDataTest, SetServerFdCreatesSocket) {
    serverData.set_server_fd();
    EXPECT_GT(serverData.get_server_fd(), 0);
}

// TEST_F(ServerDataTest, ServerBindFails) {
//     serverData.set_server_fd();
//     // `bind` のエラーを誘発するために無効なアドレスを設定
//     struct sockaddr_in invalid_address = {};
//     invalid_address.sin_family = AF_INET;
//     invalid_address.sin_addr.s_addr = INADDR_NONE;
//     invalid_address.sin_port = htons(PORT);
    
//     memcpy(&serverData.get_address(), &invalid_address, sizeof(struct sockaddr_in));
//     EXPECT_EXIT(serverData.server_bind(), ::testing::ExitedWithCode(EXIT_FAILURE), "bind failed");
// }

TEST_F(ServerDataTest, ServerListenFails) {
    EXPECT_EXIT(serverData.server_listen(), ::testing::ExitedWithCode(EXIT_FAILURE), "listen");
}

TEST_F(ServerDataTest, ServerAcceptFails) {
    EXPECT_EXIT(serverData.server_accept(), ::testing::ExitedWithCode(EXIT_FAILURE), "accept");
}

TEST_F(ServerDataTest, SetNewSocket) {
    serverData.set_new_socket(10);
    EXPECT_EQ(serverData.get_new_socket(), 10);
}
