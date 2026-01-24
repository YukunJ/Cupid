#include <cassert>
#include <algorithm>
#include <utility>
#include <vector>
#include "default_engine.h"
#include "engine_interface.h"
#include "engine_types.h"

namespace cupid {

default_engine::default_engine() : next_orderid{1} {};

std::pair<orderid_t, std::vector<execution_t>> default_engine::limit(order_t order) noexcept {
  std::vector<execution_t> execs;
  orderid_t curr_id = next_orderid++;
  order.id = curr_id;
  price_t px = order.px;
  assert(order.qty > 0);
  if (order.side == side_t::bid) {
    for (auto ask_it = ask_side.begin(); ask_it != ask_side.end();) {
      if (ask_it->px > px) {
        // bid order rest on book
        break;
      } else {
        // match happens
        price_t traded_px = ask_it->px;
        quantity_t traded_qty = std::min(ask_it->qty, order.qty);
        execs.push_back({ask_it->id, traded_px, traded_qty, side_t::ask, ask_it->instr, ask_it->trader});
        execs.push_back({curr_id, traded_px, traded_qty, side_t::bid, order.instr, order.trader});
        order.qty -= traded_qty;
        ask_it->qty -= traded_qty;
        if (ask_it->qty == 0) {
          ask_it = ask_side.erase(ask_it);
        }
        if (order.qty == 0) {
          // incoming order fully executed
          break;
        }
      }
    }
  } else {
    for (auto bid_it = bid_side.begin(); bid_it != bid_side.end();) {
      if (bid_it->px < px) {
        // ask order rest on book
        break;
      } else {
        // match happens
        price_t traded_px = bid_it->px;
        quantity_t traded_qty = std::min(bid_it->qty, order.qty);
        execs.push_back({bid_it->id, traded_px, traded_qty, side_t::bid, bid_it->instr, bid_it->trader});
        execs.push_back({curr_id, traded_px, traded_qty, side_t::ask, order.instr, order.trader});
        order.qty -= traded_qty;
        bid_it->qty -= traded_qty;
        if (bid_it->qty == 0) {
          bid_it = bid_side.erase(bid_it);
        }
        if (order.qty == 0) {
          // incoming order fully executed
          break;
        }
      }
    }
  }
  if (order.qty > 0) {
    // not fully executed, rest on book
    // need to keep the bid/ask_side sorted
    if (order.side == side_t::bid) {
      const auto& insert_pos = std::lower_bound(bid_side.begin(), bid_side.end(), order, [](auto& o1, auto& o2) -> bool { return o1.px > o2.px || (o1.px == o2.px && o1.id < o2.id);});
      bid_side.insert(insert_pos, order);
    } else {
      const auto& insert_pos = std::lower_bound(ask_side.begin(), ask_side.end(), order, [](auto& o1, auto& o2) -> bool { return o1.px < o2.px || (o1.px == o2.px && o1.id < o2.id);});
      ask_side.insert(insert_pos, order);
    }
  }
  return {curr_id, execs};
}

bool default_engine::cancel(orderid_t orderid) noexcept {
  auto it = std::find_if(bid_side.begin(), bid_side.end(), [orderid](const order_t &this_order) { return this_order.id == orderid; });
  if (it != bid_side.end()) {
    bid_side.erase(it);
    return true;
  }
  it = std::find_if(ask_side.begin(), ask_side.end(), [orderid](const order_t &this_order) { return this_order.id == orderid; });
  if (it != ask_side.end()) {
    ask_side.erase(it);
    return true;
  }
  return false;
}

}  // namespace cupid
