#pragma once
#include "../../abstract/BaseClient.hpp"
#include "../package/MuduoClient.hpp"

namespace rpc {
    class ClientFactory {
    public:
        template<typename ...Args>
        static BaseClient::ptr create(Args &&...args) {
            return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
        }
    };
}