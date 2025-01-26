# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.14)

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

# Enable Google test
FetchContent_MakeAvailable(googletest)

# For gcov to obtain test coverage: add an option to output a file
add_compile_options(--coverage)
add_link_options(--coverage)

enable_testing()
include(GoogleTest)

# includeパスを指定
include_directories("{CMAKE_SOURCE_DIR}/srcs")
include_directories("{CMAKE_SOURCE_DIR}/srcs/OSInit")

# debug用
message(STATUS "Current source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Path to srcs: ${CMAKE_SOURCE_DIR}/srcs")

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
