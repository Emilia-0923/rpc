#pragma once
#include "../../abstract/BaseClient.hpp"

namespace rpc {
    class ClientFactory {
    public:
        template<typename ...Args>
        static BaseClient::ptr create(Args &&...args) {
            return std::make_shared<BaseClient>(std::forward<Args>(args)...);
        }
    };
}