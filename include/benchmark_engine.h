#ifndef INCLUDE_BENCHMARK_ENGINE_H_
#define INCLUDE_BENCHMARK_ENGINE_H_

#include <set>
#include <utility>
#include <vector>
#include "engine_interface.h"
#include "engine_types.h"

namespace cupid {
// This is a basic implementation used as the benchmark lower bound
class benchmark_engine : public engine_interface<benchmark_engine> {
 public:
  benchmark_engine();
  std::pair<orderid_t, std::vector<execution_t>> limit(order_t order) noexcept;
  bool cancel(orderid_t orderid) noexcept;

 private:
  orderid_t next_orderid;
  std::set<order_t> bid_side;
  std::set<order_t> ask_side;
};

}  // namespace cupid

#endif  // INCLUDE_BENCHMARK_ENGINE_H_
