#include <gtest/gtest.h>

#include "adder.h"

// Demonstrate some basic assertions.
TEST(AdderTests, BasicAdderTest) {
  int x = 5;
  int y = 3;
  EXPECT_EQ(Adder{x}.add(y), x + y);
}