#include <gtest/gtest.h>

#include "webserv.hpp"

/** Demonstrate some basic assertions.
 * @see https://google.github.io/googletest/quickstart-cmake.html
 */
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

// ソースコードが存在しない際に，単体テストの分岐網羅率を100%にするためのコード．
// いずれ削除する
TEST(ABSTest, Positive) {
  EXPECT_EQ(ft::abs(4), 4);
  EXPECT_EQ(ft::abs(42), 42);
}

// ソースコードが存在しない際に，単体テストの分岐網羅率を100%にするためのコード．
// いずれ削除する
TEST(ABSTest, Negative) {
  EXPECT_EQ(ft::abs(-4), 4);
  EXPECT_EQ(ft::abs(-42), 42);
}

TEST(WebservTest, NoClash) {
  int argc = 1;
  char *argv[] = {(char *)"./webserv"};
  EXPECT_EQ(webserv(argc, argv), EXIT_SUCCESS);
}
