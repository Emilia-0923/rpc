#pragma once
#include "BaseBuffer.hpp"
#include "BaseMessage.hpp"

namespace rpc
{
    class BaseProtocol
    {
    public:
        using ptr = std::shared_ptr<BaseProtocol>;

        virtual bool can_process(const BaseBuffer::ptr& buffer) = 0;
        virtual bool on_message(const BaseBuffer::ptr& buffer, BaseMessage::ptr& msg) = 0;
        virtual std::string serialize(const BaseMessage::ptr& msg) = 0;
    };
}