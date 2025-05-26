#pragma once

#include "../muduo/package/Buffer.hpp"
#include "../../abstract/BaseBuffer.hpp"
#include <cstring>
#include <arpa/inet.h>


namespace rpc
{
    class MuduoBuffer : public BaseBuffer {
    private:
        muduo::Buffer* buffer;
    public:
        using ptr = std::shared_ptr<MuduoBuffer>;

        MuduoBuffer(muduo::Buffer* buffer) : buffer(buffer) {}

        virtual size_t read_able_size() override {
            return buffer->read_able_size();
        }

        virtual int32_t peek_int32() override {
            if (buffer->read_able_size() < sizeof(int32_t)) {
                logging.error("缓冲区数据不足，无法读取int32_t类型数据");
            }
            int32_t value;
            ::memcpy(&value, buffer->get_read_idx(), sizeof(int32_t));
            return ntohl(value); // 网络字节序转主机字节序
        }

        virtual void retrieve_int32(int32_t& _data) override {
            if (buffer->read_able_size() < sizeof(int32_t)) {
                logging.error("缓冲区数据不足，无法读取int32_t类型数据");
            }
            ::memcpy(&_data, buffer->get_read_idx(), sizeof(int32_t));
            buffer->move_read(sizeof(int32_t));
        }

        virtual int32_t read_int32() override {
            int32_t value;
            retrieve_int32(value);
            return value;
        }

        virtual std::string retrieve_as_string(size_t len) override {
            if (buffer->read_able_size() < len) {
                logging.error("缓冲区数据不足，无法读取指定长度的字符串");
            }
            std::string str(buffer->get_read_idx(), len);
            buffer->move_read(len);
            return str;
        }
    };
}