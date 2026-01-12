#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

#include <cstdint>
#include <array>
#include <type_traits>


namespace cupid {

constexpr static std::size_t CACHE_LINE_SIZE = 64;

using orderid_t = uint64_t;
enum class side : int8_t {
    invalid = 0,
    ask     = -1,
    bid     = 1
};
using side_t = side;
using quantity_t = uint32_t;
// Prices are numeric fields with an implied 4 decimal places
// for example if price_t is 155000, the actual price is $15.5
// see Nasdaq example: https://nasdaqtrader.com/content/technicalsupport/specifications/TradingProducts/Ouch5.0.pdf
using price_t = uint64_t;

constexpr static std::size_t INSTRUMENT_LEN = 4;
using instr_t = std::array<char, INSTRUMENT_LEN>;

constexpr static std::size_t TRADER_LEN = 4;
using trader_t = std::array<char, TRADER_LEN>;

struct order {
    orderid_t id; // filled and assigned after order acceptance
    price_t px;
    quantity_t qty;
    side_t side;
    instr_t instr;
    trader_t trader;

    bool operator==(const order&) const = default;
};
static_assert(sizeof(order) <= CACHE_LINE_SIZE);
static_assert(std::is_standard_layout_v<order>); 
static_assert(std::is_trivially_copyable_v<order>);

using order_t = order;
using execution_t = order_t;

}

#endif // ENGINE_TYPES_H