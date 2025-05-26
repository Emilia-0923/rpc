#pragma once

#include "../package/MuduoBuffer.hpp"

namespace rpc
{
    class BufferFactory {
    public:
        template <typename ...Args>
        static BaseBuffer::ptr create(Args&& ...args) {
            return std::make_shared<MuduoBuffer>(std::forward(args));
        }
    };
}