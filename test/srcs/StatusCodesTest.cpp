#include <gtest/gtest.h>

#include "StatusCodes.hpp"

class StatusCodesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // テストの前処理
  }

  void TearDown() override {
    // テストの後処理
  }
};

TEST_F(StatusCodesTest, ValidStatusCode200) {
  StatusCodes status;
  EXPECT_EQ(status.isValid(200), true);
}

TEST_F(StatusCodesTest, InvalidStatusCode999) {
  StatusCodes status;
  EXPECT_EQ(status.isValid(999), false);
}

TEST_F(StatusCodesTest, CheckSuccessCode) {
  StatusCodes status;
  EXPECT_EQ(status.isSuccess(200), true);
  EXPECT_EQ(status.isSuccess(404), false);
}

TEST_F(StatusCodesTest, CheckErrorCode) {
  StatusCodes status;
  EXPECT_EQ(status.isError(500), true);
  EXPECT_EQ(status.isError(200), false);
}

TEST_F(StatusCodesTest, StatusCodeMessage) {
  StatusCodes status;
  EXPECT_EQ(status.getMessage(200), "OK");
  EXPECT_EQ(status.getMessage(404), "Not Found");
  EXPECT_EQ(status.getMessage(500), "Internal Server Error");
}
