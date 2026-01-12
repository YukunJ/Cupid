#include <vector>
#include <utility>
#include "engine_types.h"
#include "engine_interface.h"
#include "default_engine.h"

// Nasdaq match engine spec section F: https://www.sec.gov/files/rules/other/nasdaqllcf1a4_5/e_sysdesc.pdf
namespace cupid {

default_engine::default_engine(): next_orderid{1} {};

std::pair<orderid_t, std::vector<execution_t>> default_engine::limit(order_t order) noexcept {
    // to fill
    return {next_orderid++, {}};
}

bool default_engine::cancel(orderid_t orderid) noexcept {
    // to fill
    return true;
}

}