#include <gtest/gtest.h>

#include "engine_types.h"
#include "engine_interface.h"
#include "default_engine.h"

namespace cupid {

TEST(DefaultEngineTests, SimpleFillTest) {
  default_engine engine;
  EXPECT_EQ(1+1, 2);
}

}