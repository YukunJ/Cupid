#ifndef INCLUDE_DEFAULT_ENGINE_H_
#define INCLUDE_DEFAULT_ENGINE_H_

#include <utility>
#include <vector>
#include "engine_interface.h"
#include "engine_types.h"

namespace cupid {

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
