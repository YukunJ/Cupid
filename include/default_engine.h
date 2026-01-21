#ifndef INCLUDE_DEFAULT_ENGINE_H_
#define INCLUDE_DEFAULT_ENGINE_H_

#include <set>
#include <utility>
#include <vector>
#include "engine_interface.h"
#include "engine_types.h"

namespace cupid {

// Nasdaq match engine spec section F:
// https://www.sec.gov/files/rules/other/nasdaqllcf1a4_5/e_sysdesc.pdf
class default_engine : public engine_interface<default_engine> {
 public:
  default_engine();
  std::pair<orderid_t, std::vector<execution_t>> limit(order_t order) noexcept;
  bool cancel(orderid_t orderid) noexcept;

 private:
  orderid_t next_orderid;
};

}  // namespace cupid

#endif  // INCLUDE_DEFAULT_ENGINE_H_
