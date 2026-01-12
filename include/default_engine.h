#ifndef DEFAULT_ENGINE_H
#define DEFAULT_ENGINE_H

#include <vector>
#include <utility>
#include "engine_types.h"
#include "engine_interface.h"

namespace cupid {

class default_engine: public engine_interface<default_engine> {
public:
    default_engine();
    std::pair<orderid_t, std::vector<execution_t>> limit(order_t order) noexcept;
    bool cancel(orderid_t orderid) noexcept;
private:
    orderid_t next_orderid;
};

}

#endif // DEFAULT_ENGINE_H