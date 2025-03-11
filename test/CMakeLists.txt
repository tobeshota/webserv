# CMake バージョンの設定
cmake_minimum_required(VERSION 3.14)

# プロジェクト設定
project(WebservTests LANGUAGES CXX)

# C++標準を指定
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GoogleTestの取得と設定
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# Windows対応: 親プロジェクトの設定を上書きしない
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# GoogleTestの利用を有効化
FetchContent_MakeAvailable(googletest)

# テスト用のカバレッジオプションを追加
add_compile_options(--coverage)
add_link_options(--coverage)

# テストフレームワークとテスト探索を有効化
enable_testing()
include(GoogleTest)

# ソースとテストコードのパス設定
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/../srcs)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/srcs)

# ソースファイルを収集
file(GLOB_RECURSE SRC_FILES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

# main.cpp を除外
list(FILTER SRC_FILES EXCLUDE REGEX "main.cpp")
list(FILTER TEST_FILES EXCLUDE REGEX "main.cpp")

# 各テスト用のターゲットを作成
foreach(test_source ${TEST_FILES})
  # テストの名前を取得
  get_filename_component(test_name ${test_source} NAME_WE)

  # テストターゲットを作成
  add_executable(${test_name} ${test_source} ${SRC_FILES})

  # インクルードパスをターゲットに設定
  target_include_directories(${test_name} PRIVATE 
    ${SOURCE_DIR}
  )

  # GoogleTest をリンク
  target_link_libraries(${test_name} GTest::gtest_main)

  # テストを登録
  gtest_discover_tests(${test_name})
endforeach()
