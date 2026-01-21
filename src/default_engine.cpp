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
  return {curr_id, execs};
}

bool default_engine::cancel(orderid_t orderid) noexcept { return false; }

}  // namespace cupid
