#pragma once

#include "../package/MuduoConnection.hpp"

namespace rpc {
    class ConnectionFactory {
    public:
        template <typename ...Args>
        static BaseConnection::ptr create(Args&& ...args) {
            return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
        }
    };
}