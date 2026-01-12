#include <gtest/gtest.h>

#include "engine_types.h"
#include "engine_interface.h"
#include "default_engine.h"

namespace cupid {

constexpr instr_t instr = {'A', 'A', 'P', 'L'};
constexpr trader_t a1 = {'A', '1', '\0', '\0'};
constexpr trader_t a2 = {'A', '2', '\0', '\0'};
constexpr trader_t b1 = {'B', '1', '\0', '\0'};
constexpr trader_t b2 = {'B', '2', '\0', '\0'};

TEST(DefaultEngineTests, SimpleFillTest) {
  default_engine engine;

  const auto& [id1, exec1] = engine.limit({0, 990000, 100, side_t::bid, instr, b1});
  EXPECT_EQ(id1, 1);
  ASSERT_TRUE(exec1.empty());

  const auto& [id2, exec2] = engine.limit({0, 1000000, 200, side_t::ask, instr, a1});
  EXPECT_EQ(id2, 2);
  ASSERT_TRUE(exec2.empty());

  // $99 @ 100 / $100 @ 200
  // full fill on bid side
  const auto& [id3, exec3] = engine.limit({0, 980000, 100, side_t::ask, instr, a1});
  EXPECT_EQ(id3, 3);
  ASSERT_EQ(exec3.size(), 2);
  EXPECT_EQ(exec3[0], execution_t({1, 990000, 100, side_t::bid, instr, b1}));
  EXPECT_EQ(exec3[1], execution_t({3, 990000, 100, side_t::ask, instr, a1}));

  // empty @ 0 / $100 @ 200
  // partial fill on ask side
  const auto& [id4, exec4] = engine.limit({0, 1000000, 100, side_t::bid, instr, b2});
  EXPECT_EQ(id4, 4);
  ASSERT_EQ(exec4.size(), 2);
  EXPECT_EQ(exec4[0], execution_t({2, 1000000, 100, side_t::ask, instr, a1}));
  EXPECT_EQ(exec4[1], execution_t({4, 1000000, 100, side_t::bid, instr, b2}));
}

}