#include <gtest/gtest.h>

/** Demonstrate some basic assertions.
 * @see https://google.github.io/googletest/quickstart-cmake.html
 */
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}
