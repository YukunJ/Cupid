// CRTP

#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H

#include "engine_types.h"
#include <vector>
#include <utility>

namespace cupid {

template <typename Impl>
class engine_interface {
public:
    // TODO: get rid of the dynamic array std::vector for reporting how many resting orders are executed
    // the resting-on-book order should be reported first in the returned vector
    [[nodiscard]] std::pair<orderid_t, std::vector<execution_t>> limit(order_t order) noexcept { return impl().limit(order); }
    bool cancel(orderid_t orderid) noexcept { return impl().cancel(orderid); }

protected:
    engine_interface() = default;
    ~engine_interface() = default;
private:
    Impl& impl() {
        return static_cast<Impl&>(*this);
    }

    const Impl& impl() const {
        return static_cast<const Impl&>(*this);
    }
};

}
#endif // ENGINE_INTERFACE_H