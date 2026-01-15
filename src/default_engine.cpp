#include "default_engine.h"
#include <utility>
#include <vector>
#include "engine_interface.h"
#include "engine_types.h"

namespace cupid {

default_engine::default_engine() : next_orderid{1} {};

std::pair<orderid_t, std::vector<execution_t>> default_engine::limit(order_t order) noexcept {
  // to fill
  std::vector<execution_t> execs;
  orderid_t curr_id = next_orderid++;
  order.id = curr_id;
  price_t px = order.px;
  if (order.side == side_t::bid) {
    for (auto ask_it = ask_side.begin(); ask_it != ask_side.end();) {
      if (ask_it->px > px) {
        // bid order rest on book
        break;
      } else {
        // match happen
        price_t traded_px = ask_it->px;
        quantity_t traded_qty = std::min(ask_it->qty, order.qty);
        execs.push_back({ask_it->id, traded_px, traded_qty, side_t::ask, ask_it->instr, ask_it->trader});
        execs.push_back({curr_id, traded_px, traded_qty, side_t::bid, order.instr, order.trader});
        auto node = ask_side.extract(ask_it++);
        node.value().qty -= traded_qty;
        // ask_it->qty -= traded_qty;
        order.qty -= traded_qty;
        if (node.value().qty == 0) {
          // fully filled
        } else {
          ask_side.insert(std::move(node));
        }
        if (order.qty == 0) {
          // full executed
          break;
        }
      }
    }
    if (order.qty > 0) {
      bid_side.insert(order);
    }
  } else {
    for (auto bid_it = bid_side.begin(); bid_it != bid_side.end();) {
      if (bid_it->px < px) {
        // ask order rest on book
        break;
      } else {
        // match happen
        price_t traded_px = bid_it->px;
        quantity_t traded_qty = std::min(bid_it->qty, order.qty);
        execs.push_back({bid_it->id, traded_px, traded_qty, side_t::bid, bid_it->instr, bid_it->trader});
        execs.push_back({curr_id, traded_px, traded_qty, side_t::ask, order.instr, order.trader});
        // bid_it->qty -= traded_qty;
        auto node = bid_side.extract(bid_it++);
        node.value().qty -= traded_qty;
        order.qty -= traded_qty;
        if (node.value().qty == 0) {
          // fully filled
        } else {
          bid_side.insert(std::move(node));
        }
        if (order.qty == 0) {
          // full executed
          break;
        }
      }
    }
    if (order.qty > 0) {
      ask_side.insert(order);
    }
  }
  return {curr_id, execs};
}

bool default_engine::cancel(orderid_t orderid) noexcept {
  for (auto it = bid_side.begin(); it != bid_side.end();) {
    if (it->id == orderid) {
      bid_side.erase(it);
      return true;
    } else {
      ++it;
    }
  }
  for (auto it = ask_side.begin(); it != ask_side.end();) {
    if (it->id == orderid) {
      ask_side.erase(it);
      return true;
    } else {
      ++it;
    }
  }
  return false;
}

}  // namespace cupid
