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
            // 如果params字段不存在，或者类型不为string，则返回false
            if (!params_field || params_field->cpp_type() != PBFieldDescriptor::CPPTYPE_STRING || !params_field->is_repeated()) {
                logging.error("RpcRequest 参数字段不存在或类型不是repeated string!");
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

        std::vector<std::string> get_params() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* params_field = descriptor->FindFieldByName(key::params);
            const PBReflection* reflection = message->GetReflection();
            
            std::vector<std::string> params_vec;
            int field_size = reflection->FieldSize(*message, params_field);
            
            // 遍历 repeated 字段中的每个元素
            for (int i = 0; i < field_size; ++i) {
                std::string param = reflection->GetRepeatedString(*message, params_field, i);
                params_vec.push_back(param);
            }
            
            return params_vec;
        }

        void set_params(const std::vector<std::string>& _params) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* params_field = descriptor->FindFieldByName(key::params);
            const PBReflection* reflection = message->GetReflection();
            // 清空现有的 repeated 字段
            reflection->ClearField(message.get(), params_field);
            // 逐个添加新的参数到 repeated 字段
            for (const auto& param : _params) {
                reflection->AddString(message.get(), params_field, param);
            }
        }
    };
}