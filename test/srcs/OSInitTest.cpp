// #include <gtest/gtest.h>

// #include "OSInit.hpp"
// #include "RunServer.hpp"
// #include "ServerData.hpp"

// // OSInit のサーバー初期化テスト
// TEST(OSInitTest, InitServer) {
//   OSInit osinit;
//   ServerData server_data;

//   ASSERT_NO_FATAL_FAILURE(osinit.initServer(server_data));
//   EXPECT_NE(server_data.get_server_fd(), -1);
// }

// // poll_fd のセットアップテスト
// TEST(OSInitTest, SetServerPollData) {
//   OSInit osinit;
//   ServerData server_data;
//   RunServer run_server;

//   server_data.set_server_fd();
//   osinit.set_serverpoll_data(server_data, run_server);

//   ASSERT_FALSE(run_server.get_poll_fds().empty());
//   EXPECT_EQ(run_server.get_poll_fds()[0].fd, server_data.get_server_fd());
//   EXPECT_EQ(run_server.get_poll_fds()[0].events, POLLIN);
// }

// // サーバーソケットのクローズテスト
// TEST(OSInitTest, CloseServerFd) {
//   OSInit osinit;
//   ServerData server_data;
//   server_data.set_server_fd();

//   ASSERT_NE(server_data.get_server_fd(), -1);
//   osinit.close_server_fd(server_data);

//   EXPECT_EQ(close(server_data.get_server_fd()), -1);  // すでに閉じているか確認
// }

// // 最大接続数のテスト
// TEST(OSInitTest, MaxConnectionsTest) {
//   OSInit osinit;
//   ServerData server_data;
//   RunServer run_server;

//   osinit.initServer(server_data);
//   osinit.set_serverpoll_data(server_data, run_server);

//   for (int i = 0; i < MAX_CONNECTION; i++) {
//     pollfd new_fd;
//     new_fd.fd = i + 5;  // ダミーファイルディスクリプタ
//     new_fd.events = POLLIN;
//     run_server.add_poll_fd(new_fd);
//   }

//   EXPECT_EQ(run_server.get_poll_fds().size(), MAX_CONNECTION + 1);
// }
