#pragma once
#include "BaseMessage.hpp"

namespace rpc
{
    class BaseConnection
    {
    public:
        using ptr = std::shared_ptr<BaseConnection>;

        virtual void send(const BaseMessage::ptr& msg) = 0;
        virtual void shutdown() = 0;
        virtual bool is_connected() = 0;
    };
}