#pragma once

#include "../package/LVProtocol.hpp"

namespace rpc {
    class ProtocolFactory {
    public:
        template<typename ...Args>
        static BaseProtocol::ptr create(Args&& ...args) {
            return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
        }
    };
}