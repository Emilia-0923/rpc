#pragma once

#include <arpa/inet.h>
#include "../abstract/BaseProtocol.hpp"
#include "../abstract/BaseBuffer.hpp"
#include "../factory/MessageFactory.hpp"

// |Length|VALUE|
// |Length|MsgType|IDLength|ID|Data|
namespace rpc
{
    class LVProtocol : public BaseProtocol {
    private:
        static const int32_t length_field_size = sizeof(int32_t);
        static const int32_t msgtype_field_size = sizeof(int32_t);
        static const int32_t idlength_field_size = sizeof(int32_t);
    public:
        using ptr = std::shared_ptr<LVProtocol>;
        
        // 判断缓冲区中数据是否足够处理一条消息
        virtual bool can_process(const BaseBuffer::ptr& buffer) {
            if (buffer->read_able_size() < length_field_size) {
                return false;
            }
            int32_t total_len = buffer->peek_int32();
            if (buffer->read_able_size() < (total_len + length_field_size)) {
                return false;
            }
            return true;
        }

        virtual bool on_message(const BaseBuffer::ptr& buffer, BaseMessage::ptr& message) {
            // 调用on_messsage时数据已经足够
            int32_t total_len = buffer->read_int32();
            MsgType msgtype = static_cast<MsgType>(buffer->read_int32());
            int32_t id_length = buffer->read_int32();
            int32_t body_length = total_len - msgtype_field_size - idlength_field_size - id_length;
            std::string id = buffer->retrieve_as_string(id_length);
            std::string body = buffer->retrieve_as_string(body_length);
            message = MessageFactory::create(msgtype);
            if(!message.get()) {
                logging.error("消息类型错误");
                return false;
            }
            if(!message->deserialize(body)) {
                logging.error("反序列化失败");
                return false;
            }
            message->set_type(msgtype);
            message->set_id(id);
            return true;
        }

        virtual std::string serialize(const BaseMessage::ptr& message) {
            std::string body = message->serialize();
            std::string id = message->get_id();
            int32_t id_length = htonl(id.size());
            int32_t mtype = htonl(static_cast<int32_t>(message->get_type()));
            int32_t h_total_length = msgtype_field_size + idlength_field_size + id.size() + body.size();
            int32_t n_total_length = htonl(h_total_length);
            std::string result;
            result.reserve(h_total_length);
            result.append((char*)&n_total_length, length_field_size);
            result.append((char*)&mtype, msgtype_field_size);
            result.append((char*)&id_length, idlength_field_size);
            result.append(id);
            result.append(body);
            return result;
        }
    };
}