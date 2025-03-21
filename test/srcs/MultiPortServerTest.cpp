// #include <gtest/gtest.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>

// #include <algorithm>
// #include <vector>

// #include "MultiPortServer.hpp"

// // MultiPortServerのテストフィクスチャ
// class MultiPortServerTest : public ::testing::Test {
// protected:
//     void SetUp() override {
//         // テスト前に実行
//         // 使用するポート番号が他のプロセスで使われていないことを確認
//         usedPorts.clear();
//     }

//     void TearDown() override {
//         // テスト後にすべてのポートをクリーンアップ
//         for (int port : usedPorts) {
//             // システムに残っているかもしれないポートをクリーンアップ
//             cleanupPort(port);
//         }
//     }

//     // テスト用のポートが使用可能か確認
//     bool isPortAvailable(int port) {
//         int sock = socket(AF_INET, SOCK_STREAM, 0);
//         if (sock < 0) return false;

//         struct sockaddr_in addr;
//         addr.sin_family = AF_INET;
//         addr.sin_addr.s_addr = INADDR_ANY;
//         addr.sin_port = htons(port);

//         int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
//         close(sock);

//         return result == 0;
//     }

//     // テスト用のポートをクリーンアップ
//     void cleanupPort(int port) {
//         // システムコマンドを使用して強制的にポートを解放
//         std::string cmd = "fuser -k " + std::to_string(port) + "/tcp
//         >/dev/null 2>&1 || true"; system(cmd.c_str());
//     }

//     // 使用したポート番号を追跡
//     std::vector<int> usedPorts;

//     // テスト用の利用可能なポートを取得
//     int getAvailablePort() {
//         // テスト用のポート範囲（49152-65535が動的・私用ポート）
//         int start_port = 55000; // 高いポート番号からスタート
//         int end_port = 65000;

//         for (int port = start_port; port <= end_port; ++port) {
//             if (isPortAvailable(port) &&
//                 std::find(usedPorts.begin(), usedPorts.end(), port) ==
//                 usedPorts.end()) { usedPorts.push_back(port); return port;
//             }
//         }
//         return -1; // 利用可能なポートが見つからない
//     }

//     // 複数の利用可能なポートを取得
//     std::vector<int> getAvailablePorts(size_t count) {
//         std::vector<int> ports;
//         for (size_t i = 0; i < count; ++i) {
//             int port = getAvailablePort();
//             if (port != -1) {
//                 ports.push_back(port);
//             } else {
//                 break;
//             }
//         }
//         return ports;
//     }
// };

// // コンストラクタのテスト
// TEST_F(MultiPortServerTest, Constructor) {
//     MultiPortServer server;

//     // 初期状態では空のポートリストとサーバーFDリストを持つはず
//     EXPECT_TRUE(server.getPorts().empty());
//     EXPECT_TRUE(server.getServerFds().empty());
// }

// // addPortメソッドのテスト
// TEST_F(MultiPortServerTest, AddPort) {
//     MultiPortServer server;

//     // ポートを追加
//     int testPort1 = 8080;
//     int testPort2 = 8081;

//     server.addPort(testPort1);
//     EXPECT_EQ(server.getPorts().size(), 1);
//     EXPECT_EQ(server.getPorts()[0], testPort1);

//     server.addPort(testPort2);
//     EXPECT_EQ(server.getPorts().size(), 2);
//     EXPECT_EQ(server.getPorts()[1], testPort2);
// }

// // setPortsメソッドのテスト
// TEST_F(MultiPortServerTest, SetPorts) {
//     MultiPortServer server;

//     // 複数のポートを一度に設定
//     std::vector<int> testPorts = {8080, 8081, 8082};
//     server.setPorts(testPorts);

//     // ポートリストが正しく設定されていることを確認
//     EXPECT_EQ(server.getPorts().size(), testPorts.size());
//     for (size_t i = 0; i < testPorts.size(); ++i) {
//         EXPECT_EQ(server.getPorts()[i], testPorts[i]);
//     }

//     // 再設定した場合、前の設定が上書きされることを確認
//     std::vector<int> newPorts = {9090, 9091};
//     server.setPorts(newPorts);

//     EXPECT_EQ(server.getPorts().size(), newPorts.size());
//     for (size_t i = 0; i < newPorts.size(); ++i) {
//         EXPECT_EQ(server.getPorts()[i], newPorts[i]);
//     }
// }

// // initializeSocketsメソッドの基本テスト
// TEST_F(MultiPortServerTest, InitializeSockets) {
//     MultiPortServer server;

//     // 利用可能なポートを取得
//     std::vector<int> availablePorts = getAvailablePorts(2);
//     ASSERT_GE(availablePorts.size(), 2) <<
//     "テスト用の利用可能なポートが不足しています";

//     // ポートを設定
//     server.setPorts(availablePorts);

//     // ソケット初期化
//     bool result = server.initializeSockets();
//     EXPECT_TRUE(result);

//     // サーバーFDが正しく作成されたか確認
//     EXPECT_EQ(server.getServerFds().size(), availablePorts.size());

//     // 各サーバーFDが有効であることを確認
//     for (size_t i = 0; i < server.getServerFds().size(); ++i) {
//         EXPECT_GT(server.getServerFds()[i], 0);
//     }

//     // 各ポートのマッピングが正しいか確認
//     for (size_t i = 0; i < server.getServerFds().size(); ++i) {
//         int serverFd = server.getServerFds()[i];
//         EXPECT_TRUE(server.isServerFd(serverFd));
//         EXPECT_EQ(server.getPortByFd(serverFd), availablePorts[i]);
//     }
// }

// // initializeSocketsメソッドのエラーケース（無効なポート）
// TEST_F(MultiPortServerTest, InitializeSocketsWithInvalidPort) {
//     MultiPortServer server;

//     // 権限のない特権ポート（1-1023）を使用
//     std::vector<int> invalidPorts = {1, 22, 80};
//     server.setPorts(invalidPorts);

//     //
//     権限がなければ失敗するはず（ただし、rootで実行すると成功する可能性がある）
//     bool result = server.initializeSockets();

//     // 結果はテスト環境によって異なるため、ここでは特定の結果を期待しない
//     // 代わりに、結果に関わらずサーバーFDのリストが空でないことを確認
//     if (result) {
//         EXPECT_FALSE(server.getServerFds().empty());
//     }
// }

// // isServerFd関数のテスト
// TEST_F(MultiPortServerTest, IsServerFd) {
//     MultiPortServer server;

//     // 利用可能なポートを取得
//     int availablePort = getAvailablePort();
//     ASSERT_NE(availablePort, -1) <<
//     "テスト用の利用可能なポートが見つかりません";

//     // ポートを設定して初期化
//     server.addPort(availablePort);
//     bool result = server.initializeSockets();
//     EXPECT_TRUE(result);

//     // 有効なサーバーFDを確認
//     int validFd = server.getServerFds()[0];
//     EXPECT_TRUE(server.isServerFd(validFd));

//     // 無効なFDの場合はfalseを返すはず
//     EXPECT_FALSE(server.isServerFd(-1));
//     EXPECT_FALSE(server.isServerFd(0));
//     EXPECT_FALSE(server.isServerFd(999999)); // 存在しないはずの大きな値
// }

// // getPortByFd関数のテスト
// TEST_F(MultiPortServerTest, GetPortByFd) {
//     MultiPortServer server;

//     // 利用可能なポートを取得
//     std::vector<int> availablePorts = getAvailablePorts(2);
//     ASSERT_GE(availablePorts.size(), 2) <<
//     "テスト用の利用可能なポートが不足しています";

//     // ポートを設定して初期化
//     server.setPorts(availablePorts);
//     bool result = server.initializeSockets();
//     EXPECT_TRUE(result);

//     // 各FDに対して正しいポート番号が返されることを確認
//     for (size_t i = 0; i < server.getServerFds().size(); ++i) {
//         int fd = server.getServerFds()[i];
//         EXPECT_EQ(server.getPortByFd(fd), availablePorts[i]);
//     }

//     // 無効なFDの場合は-1を返すはず
//     EXPECT_EQ(server.getPortByFd(-1), -1);
//     EXPECT_EQ(server.getPortByFd(0), -1);
//     EXPECT_EQ(server.getPortByFd(999999), -1); // 存在しないはずの大きな値
// }

// // closeSocketsメソッドのテスト
// TEST_F(MultiPortServerTest, CloseSockets) {
//     MultiPortServer server;

//     // 利用可能なポートを取得
//     int availablePort = getAvailablePort();
//     ASSERT_NE(availablePort, -1) <<
//     "テスト用の利用可能なポートが見つかりません";

//     // ポートを設定して初期化
//     server.addPort(availablePort);
//     bool result = server.initializeSockets();
//     EXPECT_TRUE(result);

//     // 初期化後はサーバーFDが存在するはず
//     EXPECT_FALSE(server.getServerFds().empty());

//     // ソケットをクローズ
//     server.closeSockets();

//     // サーバーFDリストが空になっているはず
//     EXPECT_TRUE(server.getServerFds().empty());
// }

// // デストラクタが正しく動作することを確認（間接的に）
// TEST_F(MultiPortServerTest, Destructor) {
//     // スコープ内でサーバーを作成して初期化
//     {
//         MultiPortServer server;

//         // 利用可能なポートを取得
//         int availablePort = getAvailablePort();
//         ASSERT_NE(availablePort, -1) <<
//         "テスト用の利用可能なポートが見つかりません";

//         // ポートを設定して初期化
//         server.addPort(availablePort);
//         bool result = server.initializeSockets();
//         EXPECT_TRUE(result);

//         // ここでスコープを抜けるとデストラクタが呼ばれる
//     }

//     // デストラクタが呼ばれた後、ポートが解放されていることを確認
//     // 同じポートで新しいサーバーを作成できるはず
//     int lastUsedPort = usedPorts.back();

//     MultiPortServer newServer;
//     newServer.addPort(lastUsedPort);
//     bool result = newServer.initializeSockets();

//     // ポートが正しく解放されていれば成功するはず
//     EXPECT_TRUE(result);
// }

// // 複数回の初期化テスト
// TEST_F(MultiPortServerTest, MultipleInitialization) {
//     MultiPortServer server;

//     // 1回目の初期化
//     std::vector<int> ports1 = getAvailablePorts(2);
//     ASSERT_GE(ports1.size(), 2) <<
//     "テスト用の利用可能なポートが不足しています";

//     server.setPorts(ports1);
//     bool result1 = server.initializeSockets();
//     EXPECT_TRUE(result1);

//     // サーバーFDが作成されたことを確認
//     EXPECT_EQ(server.getServerFds().size(), ports1.size());

//     // 2回目の初期化（異なるポート）
//     std::vector<int> ports2 = getAvailablePorts(3);
//     ASSERT_GE(ports2.size(), 3) <<
//     "テスト用の利用可能なポートが不足しています";

//     server.setPorts(ports2);
//     bool result2 = server.initializeSockets();
//     EXPECT_TRUE(result2);

//     // 新しいサーバーFDが作成されたことを確認
//     EXPECT_EQ(server.getServerFds().size(), ports2.size());

//     // 元のポートが解放されたことを確認
//     for (int port : ports1) {
//         EXPECT_TRUE(isPortAvailable(port));
//     }
// }

// // エッジケース：空のポートリストのテスト
// TEST_F(MultiPortServerTest, EmptyPortList) {
//     MultiPortServer server;

//     // 空のポートリストで初期化
//     std::vector<int> emptyPorts;
//     server.setPorts(emptyPorts);

//     // 初期化は失敗するはず（成功するポートがないため）
//     bool result = server.initializeSockets();
//     EXPECT_FALSE(result);

//     // サーバーFDリストも空のはず
//     EXPECT_TRUE(server.getServerFds().empty());
// }

// // エッジケース：一部のポートだけ成功するケース
// TEST_F(MultiPortServerTest, PartialSuccess) {
//     MultiPortServer server;

//     // 有効なポートと無効なポートを混ぜる
//     std::vector<int> mixedPorts;

//     // 利用可能なポートを追加
//     int availablePort = getAvailablePort();
//     ASSERT_NE(availablePort, -1) <<
//     "テスト用の利用可能なポートが見つかりません";
//     mixedPorts.push_back(availablePort);

//     // 権限のない特権ポートを追加
//     mixedPorts.push_back(22);  // SSHポート（通常は使用できない）

//     server.setPorts(mixedPorts);

//     // 少なくとも1つのポートが成功すれば、initializeSocketsはtrueを返すはず
//     bool result = server.initializeSockets();

//     // 通常のユーザー権限では少なくとも1つは成功するはず
//     EXPECT_TRUE(result);

//     // 成功したポート数はテスト環境によって変わるが、少なくとも1つはあるはず
//     EXPECT_GE(server.getServerFds().size(), 1);
// }

// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
