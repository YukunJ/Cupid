#include <vector>
#include <utility>
#include "engine_types.h"
#include "engine_interface.h"
#include "default_engine.h"

namespace cupid {

void default_engine::init() {
    // to fill
}

void default_engine::destroy() {
    // to fill
}

std::pair<orderid_t, std::vector<execution_t>> default_engine::limit(order_t order) noexcept {
    // to fill
    return {curr_orderid++, {}};
}

bool default_engine::cancel(orderid_t orderid) noexcept {
    // to fill
    return true;
}

void default_engine::execution(execution_t exec) {
    // to fill
}

}