#pragma once

#include "MuduoBuffer.hpp"

namespace rpc
{
    template <typename ...Args>
    class MuduoBufferFactory {
        static BaseBuffer::ptr create(Args&& ...args) {
            return std::make_shared<MuduoBuffer>(std::forward(args));
        }
    };
}