#include <gtest/gtest.h>

#include "default_engine.h"
#include "engine_interface.h"
#include "engine_types.h"

namespace cupid {

constexpr instr_t instr = {'A', 'A', 'P', 'L'};
constexpr trader_t a1 = {'A', '1', '\0', '\0'};
constexpr trader_t a2 = {'A', '2', '\0', '\0'};
constexpr trader_t b1 = {'B', '1', '\0', '\0'};
constexpr trader_t b2 = {'B', '2', '\0', '\0'};

TEST(DefaultEngineTests, BasicFillTest) {
  default_engine engine;

  const auto &[id1, exec1] = engine.limit({0, 990000, 100, side_t::bid, instr, b1});
  EXPECT_EQ(id1, 1);
  ASSERT_TRUE(exec1.empty());

  const auto &[id2, exec2] = engine.limit({0, 1000000, 200, side_t::ask, instr, a1});
  EXPECT_EQ(id2, 2);
  ASSERT_TRUE(exec2.empty());

  // $99 @ 100 / $100 @ 200
  // full fill on bid side
  const auto &[id3, exec3] = engine.limit({0, 980000, 100, side_t::ask, instr, a1});
  EXPECT_EQ(id3, 3);
  ASSERT_EQ(exec3.size(), 2);
  EXPECT_EQ(exec3[0], execution_t({1, 990000, 100, side_t::bid, instr, b1}));
  EXPECT_EQ(exec3[1], execution_t({3, 990000, 100, side_t::ask, instr, a1}));

  // empty @ 0 / $100 @ 200
  // partial fill on ask side
  const auto &[id4, exec4] = engine.limit({0, 1000000, 100, side_t::bid, instr, b2});
  EXPECT_EQ(id4, 4);
  ASSERT_EQ(exec4.size(), 2);
  EXPECT_EQ(exec4[0], execution_t({2, 1000000, 100, side_t::ask, instr, a1}));
  EXPECT_EQ(exec4[1], execution_t({4, 1000000, 100, side_t::bid, instr, b2}));

  // empty @ 0 / $100 @ 100
  const auto &[id5, exec5] = engine.limit({0, 995000, 150, side_t::ask, instr, a2});
  EXPECT_EQ(id5, 5);
  ASSERT_EQ(exec5.size(), 0);

  // empty @ 0 / $99.5 @ 150, $100 @ 100
  // multiple fill with 1 limit order on bid side across the spread
  const auto &[id6, exec6] = engine.limit({0, 1005000, 200, side_t::bid, instr, b1});
  EXPECT_EQ(id6, 6);
  ASSERT_EQ(exec6.size(), 4);
  EXPECT_EQ(exec6[0], execution_t({5, 995000, 150, side_t::ask, instr, a2}));
  EXPECT_EQ(exec6[1], execution_t({6, 995000, 150, side_t::bid, instr, b1}));
  EXPECT_EQ(exec6[2], execution_t({2, 1000000, 50, side_t::ask, instr, a1}));
  EXPECT_EQ(exec6[3], execution_t({6, 1000000, 50, side_t::bid, instr, b1}));

  // empty @ 0 / $100 @ 50
  // agressively clear the ask side and rest on the TOB bid side
  const auto &[id7, exec7] = engine.limit({0, 1005000, 150, side_t::bid, instr, b2});
  EXPECT_EQ(id7, 7);
  ASSERT_EQ(exec7.size(), 2);
  EXPECT_EQ(exec7[0], execution_t({2, 1000000, 50, side_t::ask, instr, a1}));
  EXPECT_EQ(exec7[1], execution_t({7, 1000000, 50, side_t::bid, instr, b2}));

  // $100.5 @ 100 / empty @ 0
  // check the bid side has the remaining quantity on TOB after last execution
  const auto &[id8, exec8] = engine.limit({0, 990000, 150, side_t::ask, instr, a1});
  EXPECT_EQ(id8, 8);
  ASSERT_EQ(exec8.size(), 2);
  EXPECT_EQ(exec8[0], execution_t({7, 1005000, 100, side_t::bid, instr, b2}));
  EXPECT_EQ(exec8[1], execution_t({8, 1005000, 100, side_t::ask, instr, a1}));
}

TEST(DefaultEngineTests, BasicCancelTest) {
  default_engine engine;

  const auto &[id1, exec1] = engine.limit({0, 990000, 100, side_t::bid, instr, b1});
  const auto &[id2, exec2] = engine.limit({0, 1000000, 200, side_t::ask, instr, a1});

  EXPECT_TRUE(engine.cancel(1));

  // cancel the same order twice
  EXPECT_FALSE(engine.cancel(1));

  // emtpy @ 0 / $100 @ 200
  const auto &[id3, exec3] = engine.limit({0, 1010000, 100, side_t::bid, instr, b1});
  EXPECT_EQ(id3, 3);
  ASSERT_EQ(exec3.size(), 2);

  // too late to cancel
  // the book should be: empty @ 0 / $100 @ 100 now
  EXPECT_FALSE(engine.cancel(3));
  EXPECT_TRUE(engine.cancel(2));
}

TEST(DefaultEngineTests, IntegratedFillCancelTest) {
  default_engine engine;

  const auto &[id1, exec1] = engine.limit({0, 990000, 100, side_t::bid, instr, b1});
  const auto &[id2, exec2] = engine.limit({0, 1000000, 200, side_t::ask, instr, a1});
  EXPECT_EQ(id1, 1);
  EXPECT_EQ(id2, 2);
  ASSERT_TRUE(exec1.empty());
  ASSERT_TRUE(exec2.empty());

  // Partial fill and cancel from same side
  // $99 @ 100 /  $100 @ 200
  const auto &[id3, exec3] = engine.limit({0, 1000000, 50, side_t::bid, instr, b2});
  EXPECT_EQ(id3, 3);
  ASSERT_EQ(exec3.size(), 2);
  EXPECT_EQ(exec3[0], execution_t({2, 1000000, 50, side_t::ask, instr, a1}));
  EXPECT_EQ(exec3[1], execution_t({3, 1000000, 50, side_t::bid, instr, b2}));

  // Cancel remaining ask side. order id 2 should still have remaining qty 50
  EXPECT_TRUE(engine.cancel(2));

  // test queue position - multiple orders at same price
  // $99 @ 100 / empty @ 0
  const auto &[id4, exec4] = engine.limit({0, 990000, 50, side_t::bid, instr, b2});
  const auto &[id5, exec5] = engine.limit({0, 990000, 75, side_t::bid, instr, b1});
  EXPECT_EQ(id4, 4);
  EXPECT_EQ(id5, 5);
  ASSERT_TRUE(exec4.empty());
  ASSERT_TRUE(exec5.empty());

  // Fill from ask side clears bid side queue in order
  // $99 @ 225 (id1:100, id4:50, id5:75) / empty @ 0
  const auto &[id6, exec6] = engine.limit({0, 990000, 120, side_t::ask, instr, a1});
  EXPECT_EQ(id6, 6);
  ASSERT_EQ(exec6.size(), 4);
  // Should fill against id1 first (100), then id4 (20)
  EXPECT_EQ(exec6[0], execution_t({1, 990000, 100, side_t::bid, instr, b1}));
  EXPECT_EQ(exec6[1], execution_t({6, 990000, 100, side_t::ask, instr, a1}));
  EXPECT_EQ(exec6[2], execution_t({4, 990000, 20, side_t::bid, instr, b2}));
  EXPECT_EQ(exec6[3], execution_t({6, 990000, 20, side_t::ask, instr, a1}));

  // Cancel middle order in queue
  // $99 @ 105 (id4:30, id5:75) / empty @ 0
  EXPECT_TRUE(engine.cancel(4));

  // Incoming order rest on book and then be filled
  // $99 @ 75 (id5:75) / empty @ 0
  const auto &[id7, exec7] = engine.limit({0, 995000, 100, side_t::ask, instr, a1});
  EXPECT_EQ(id7, 7);
  ASSERT_TRUE(exec7.empty());

  // $99 @ 75 (id5:75) / $99.5 @ 100
  const auto &[id8, exec8] = engine.limit({0, 1000000, 50, side_t::bid, instr, b1});
  EXPECT_EQ(id8, 8);
  ASSERT_EQ(exec8.size(), 2);
  EXPECT_EQ(exec8[0], execution_t({7, 995000, 50, side_t::ask, instr, a1}));
  EXPECT_EQ(exec8[1], execution_t({8, 995000, 50, side_t::bid, instr, b1}));

  // cancel after partial fill
  // $99 @ 75 (id5:75) / $99.5 @ 50
  EXPECT_TRUE(engine.cancel(7));  // cancel remaining 50 qty from ask

  // aggressive order clears multiple price levels
  // $99 @ 75 (id5:75) / empty @ 0
  const auto &[id9, exec9] = engine.limit({0, 980000, 50, side_t::bid, instr, b2});
  const auto &[id10, exec10] = engine.limit({0, 1010000, 150, side_t::ask, instr, a2});

  // $98 @ 50 (id9:50) , $99 @ 75 (id5:75) / $101 @ 150
  const auto &[id11, exec11] = engine.limit({0, 1020000, 200, side_t::bid, instr, b1});
  EXPECT_EQ(id11, 11);
  ASSERT_EQ(exec11.size(), 2);
  EXPECT_EQ(exec11[0], execution_t({10, 1010000, 150, side_t::ask, instr, a2}));
  EXPECT_EQ(exec11[1], execution_t({11, 1010000, 150, side_t::bid, instr, b1}));

  // Book: BID $98@50, $99@75, $102@50 / ASK empty

  // cancel from back of queue at same price
  // $98 @ 50 (id9:50) , $99 @ 75 (id5:75), $102 @ 50 (id11:50) / empty @ 0
  const auto &[id12, exec12] = engine.limit({0, 1020000, 25, side_t::bid, instr, b2});
  EXPECT_EQ(id12, 12);
  ASSERT_TRUE(exec12.empty());

  // cancel id11 (50 remaining) should work, leaving id12 at same price
  // $98 @ 50 (id9:50) , $99 @ 75 (id5:75), $102 @ 75 (id11:50, id12:25) / empty
  // @ 0
  EXPECT_TRUE(engine.cancel(11));

  // matching order to verify queue position
  // $98 @ 50 (id9:50) , $99 @ 75 (id5:75), $102 @ 25 (id12:25) / empty @ 0
  const auto &[id13, exec13] = engine.limit({0, 1020000, 30, side_t::ask, instr, a2});
  EXPECT_EQ(id13, 13);
  // Should fill against id12 (25) only, not against cancelled id11
  ASSERT_EQ(exec13.size(), 2);
  EXPECT_EQ(exec13[0], execution_t({12, 1020000, 25, side_t::bid, instr, b2}));
  EXPECT_EQ(exec13[1], execution_t({13, 1020000, 25, side_t::ask, instr, a2}));
}
}  // namespace cupid
