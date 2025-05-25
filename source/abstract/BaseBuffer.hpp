#pragma once
#include <string>
#include <memory>

namespace rpc
{
    class BaseBuffer
    {
    public:
        using ptr = std::shared_ptr<BaseBuffer>;

        virtual size_t read_able_size() = 0;
        virtual int32_t peek_int32() = 0;
        virtual void retrieve_int32(int32_t& _data) = 0;
        virtual int32_t read_int32() = 0;
        virtual std::string retrieve_as_string(size_t len) = 0;
    };
}