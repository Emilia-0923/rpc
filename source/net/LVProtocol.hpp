#pragma once

#include "../abstract/BaseProtocol.hpp"
#include "MuduoBuffer.hpp"
#include "MessageFactory.hpp"

// |Length|VALUE|
// |Length|MsgType|IDLength|ID|Data|
namespace rpc
{
    class LVProtocol : public BaseProtocol {
    private:
        static const size_t length_field_size = sizeof(int32_t);
        static const size_t msgtype_field_size = sizeof(int32_t);
        static const size_t idlength_field_size = sizeof(int32_t);
    public:
        using ptr = std::shared_ptr<LVProtocol>;
        
        // 判断缓冲区中数据是否足够处理一条消息
        virtual bool can_process(const BaseBuffer::ptr& buffer) {
            int32_t total_len = buffer->peek_int32();
            if (buffer->read_able_size() < (total_len + length_field_size)) {
                return false;
            }
            return true;
        }

        virtual bool on_message(const BaseBuffer::ptr& buffer, const BaseMessage::ptr& msg) {
            // 调用on_messsage时数据已经足够
            int32_t total_len = buffer->read_int32();
            int32_t msgtype = buffer->read_int32();
            int32_t idlength = buffer->read_int32();
            int32_t bodylength = total_len - length_field_size - msgtype_field_size - idlength;
            std::string id = buffer->retrieve_as_string(idlength);
            std::string body = buffer->retrieve_as_string(bodylength);
            auto msg = MessageFactory::create
        }

        virtual std::string serialize(const BaseMessage::ptr& msg) = 0;
    }
}