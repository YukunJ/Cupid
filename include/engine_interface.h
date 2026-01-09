// CRTP

#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H

#include "engine_types.h"

namespace cupid {

template <typename Impl>
class engine_interface {
public:
    void init() { impl().init(); }
    void destroy() { impl().destroy(); }
    [[nodiscard]] orderid_t limit(order_t order) noexcept { return impl().limit(order); }
    bool cancel(orderid_t orderid) noexcept { return impl().cancel(orderid); }
    void execution(execution_t exec) {impl().execution(exec); }

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