#pragma once
#include "concrete/ProtoRequest.hpp"

namespace rpc
{
    class TopicRequest : public ProtoRequest
    {
    public:
        using ptr = std::shared_ptr<TopicRequest>;

        TopicRequest() : ProtoRequest(new msg::TopicRequest) {}

        // 主题请求: 主题名称和操作类型
        virtual bool check() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* topic_field = descriptor->FindFieldByName(key::topic);
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName(key::optype);
            // 如果topic字段不存在，或者类型不为stirng，则返回false
            if(!topic_field || topic_field->type() != PBFieldDescriptor::TYPE_STRING) {
                logging.error("TopicRequest 主题名称字段不存在或类型不为string!");
                return false;
            }
            // 如果optype字段不存在，或者类型不为int32，则返回false
            if(!optype_field || optype_field->type() != PBFieldDescriptor::TYPE_INT32) {
                logging.error("TopicRequest 操作类型字段不存在或类型不正确!");
                return false;
            }
            // 如果操作类型为发布消息，则message字段必须存在且类型为string
            const PBReflection* reflection = message->GetReflection();
            if(static_cast<int>(reflection->GetInt32(*message, optype_field)) == static_cast<int>(TopicOptype::PUBLISH)) {
                // 如果message字段不存在，或者类型不为string，则返回false
                const PBFieldDescriptor* message_field = descriptor->FindFieldByName(key::message);
                if(!message_field || message_field->type() != PBFieldDescriptor::TYPE_STRING) {
                    logging.error("TopicRequest 发布消息字段不存在或类型不为string!");
                    return false;
                }
            }

            return true;
        }

        std::string get_topic() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* topic_field = descriptor->FindFieldByName(key::topic);
            return message->GetReflection()->GetString(*message, topic_field);
        }

        void set_topic(const std::string& _topic) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* topic_field = descriptor->FindFieldByName(key::topic);
            message->GetReflection()->SetString(message.get(), topic_field, _topic);
        }

        TopicOptype get_optype() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName(key::optype);
            return static_cast<TopicOptype>(message->GetReflection()->GetInt32(*message, optype_field));
        }

        void set_optype(TopicOptype _optype) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName(key::optype);
            message->GetReflection()->SetInt32(message.get(), optype_field, static_cast<int>(_optype));
        }

        std::string get_message() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* message_field = descriptor->FindFieldByName(key::message);
            return message->GetReflection()->GetString(*message, message_field);
        }

        void set_message(const std::string& _message) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* message_field = descriptor->FindFieldByName(key::message);
            message->GetReflection()->SetString(message.get(), message_field, _message);
        }
    };
}