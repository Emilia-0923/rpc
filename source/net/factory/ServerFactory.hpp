#pragma once

#include "../package/MuduoServer.hpp"

namespace rpc {
    class ServerFactory {
    public:
        template<typename ...Args>
        static BaseServer::ptr create(Args &&...args) {
            return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
        }
    };
}