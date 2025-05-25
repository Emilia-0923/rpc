#pragma once
#include "ProtoMessage.hpp"

namespace rpc
{
    class ProtoResponse : public ProtoMessage
    {
    public:
        using ptr = std::shared_ptr<ProtoResponse>;

        virtual bool check() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* retcode_field = descriptor->FindFieldByName(key::retcode);
            // 如果消息类型不包含retcode字段，或者retcode字段类型不是int32，则返回false
            if (retcode_field == nullptr || retcode_field->cpp_type() != PBFieldDescriptor::CPPTYPE_INT32) {
                logging.error("Response retcode字段不存在或类型不正确!");
                return false;
            }
            return true;
        }

        virtual RetCode get_retcode() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* retcode_field = descriptor->FindFieldByName(key::retcode);
            const PBReflection* reflection = message->GetReflection();
            return static_cast<RetCode>(reflection->GetInt32(*message, retcode_field));
        }

        virtual void set_retcode(RetCode _retcode) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* retcode_field = descriptor->FindFieldByName(key::retcode);
            const PBReflection* reflection = message->GetReflection();
            reflection->SetInt32(message, retcode_field, static_cast<int>(_retcode));
        }
    };
}