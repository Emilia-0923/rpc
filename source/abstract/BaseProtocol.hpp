#pragma once
#include "BaseBuffer.hpp"
#include "BaseMessage.hpp"

namespace rpc
{
    class BaseProtocol
    {
    public:
        virtual bool can_process(const BaseBuffer::ptr& buffer) = 0;
        virtual bool on_message(const BaseBuffer::ptr& buffer, const BaseMessage::ptr& msg) = 0;
        virtual std::string serialize(const BaseMessage::ptr& msg) = 0;
    };
}