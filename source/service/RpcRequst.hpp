#pragma once
#include "../concrete/ProtoRequest.hpp"
#include "../pbmessage/RpcRequest.pb.h"

namespace rpc
{
    class RpcRequest : public ProtoRequest
    {
    public:
        using ptr = std::shared_ptr<RpcRequest>;

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
            if (!params_field) {
                logging.error("RpcRequest 参数字段不存在!");
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
            message->GetReflection()->SetString(message, method_field, _method);
        }

        std::vector<std::string> get_params() {
            //以\3为分割符
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* params_field = descriptor->FindFieldByName(key::params);
            std::string params = message->GetReflection()->GetString(*message, params_field);
            std::vector<std::string> params_vec;
            std::string param;
            for (auto& c : params) {
                if (c == '\3') {
                    params_vec.push_back(std::move(param));
                }
                else {
                    param += c;
                }
            }
            return params_vec;
        }

        void set_params(const std::vector<std::string>& _params) {
            //以\3为分割符
            std::string params;
            for (auto& param : _params) {
                params += param + "\3";
            }
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* params_field = descriptor->FindFieldByName(key::params);
            message->GetReflection()->SetString(message, params_field, params);
        }
    };
}