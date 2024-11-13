# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.14)
# プロジェクト名と使用する言語を設定
project(my_project)

# 条件付きでTESTマクロを定義
if(DEFINED TEST_BY_GTEST)
  add_compile_definitions(TEST_BY_GTEST)  # #define TEST_BY_GTEST と同義
endif()

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# includeパスを指定
include_directories("../srcs/")

# cppファイルをビルド対象に加える
file(GLOB TEST_FILES "srcs/*.cpp")
file(GLOB SRCS_FILES "../srcs/*.cpp")
# ただし「main.cpp(main関数が含まれるファイル)」はビルド対象から外す（main関数が含まれるファイルをGoogleTestのソースファイルに用いることができないため）
list(FILTER TEST_FILES EXCLUDE REGEX "main.cpp")
list(FILTER SRCS_FILES EXCLUDE REGEX "main.cpp")

foreach(test_source ${TEST_FILES})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source} ${SRCS_FILES})
  target_link_libraries(${test_name} GTest::gtest_main)
  gtest_discover_tests(${test_name})
endforeach()
