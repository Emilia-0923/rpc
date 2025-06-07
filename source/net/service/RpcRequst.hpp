#pragma once
#include "concrete/ProtoRequest.hpp"

namespace rpc
{
    class RpcRequest : public ProtoRequest
    {
    public:
        using ptr = std::shared_ptr<RpcRequest>;

        RpcRequest() : ProtoRequest(new msg::RpcRequest()) {}

        virtual bool check() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName(key::method);
            const PBFieldDescriptor* params_field = descriptor->FindFieldByName(key::params);
            // 如果method字段不存在，或者method类型不是string，则返回false
            if (!method_field || method_field->cpp_type() != PBFieldDescriptor::CPPTYPE_STRING) {
                logging.error("RpcRequest 方法名字段不存在或类型不是string!");
                return false;
            }
            // 如果params字段存在，且类型不为PBValue，则返回false
            if (params_field && params_field->cpp_type() != PBFieldDescriptor::CPPTYPE_MESSAGE) {
                logging.error("RpcRequest 参数字段存在但类型不是PBValue!");
                return false;
            }
            return true;
        }

        std::string get_method() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName(key::method);
            return message->GetReflection()->GetString(*message, method_field);
        }

        void set_method(const std::string& _method) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName(key::method);
            message->GetReflection()->SetString(message.get(), method_field, _method);
        }

        std::vector<PBValue> get_params() {
            const auto* descriptor = message->GetDescriptor();
            const auto* params_field = descriptor->FindFieldByName(key::params);
            const auto* reflection = message->GetReflection();

            std::vector<PBValue> params_vec;
            int field_size = reflection->FieldSize(*message, params_field);
            params_vec.reserve(field_size); 

            for (int i = 0; i < field_size; ++i) {
                const auto& val = dynamic_cast<const PBValue&>(reflection->GetRepeatedMessage(*message, params_field, i));
                params_vec.push_back(val);
            }

            return params_vec;
        }

        void set_params(const std::vector<PBValue>& _params) {
            const auto* descriptor = message->GetDescriptor();
            const auto* params_field = descriptor->FindFieldByName(key::params);
            const auto* reflection = message->GetReflection();

            reflection->ClearField(message.get(), params_field);

            for (const auto& value : _params) {
                // 直接添加并复制 Value
                auto* msg = reflection->AddMessage(message.get(), params_field);
                msg->CopyFrom(value);  // Value 是 Message 的子类，可直接 CopyFrom
            }
        }
    };
}