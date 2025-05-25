#pragma once
#include "../concrete/ProtoResponse.hpp"
#include "../pbmessage/RpcResponse.pb.h"

namespace rpc
{
    class RpcResponse : public ProtoResponse
    {
    public:
        using ptr = std::shared_ptr<RpcResponse>;

        virtual bool check() override {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* retcode_field = descriptor->FindFieldByName("retcode");
            const PBFieldDescriptor* result_field = descriptor->FindFieldByName("result");
            // 返回码字段存在，并且返回码类型为int32
            if (retcode_field && retcode_field->cpp_type() == PBFieldDescriptor::CPPTYPE_INT32) {
                logging.error("RpcResponse 返回码为空或类型错误!");
                return false;
            }
            // 结果字段存在，并且结果类型为string
            if (result_field && result_field->cpp_type() == PBFieldDescriptor::CPPTYPE_STRING) {
                logging.error("RpcResponse 返回结果为空或类型错误!");
                return false;
            }
            return true;
        }

        std::string get_result() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* result_field = descriptor->FindFieldByName("result");
            const PBReflection* reflection = message->GetReflection();
            return reflection->GetString(*message, result_field);
        }

        void set_result(const std::string& _result) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* result_field = descriptor->FindFieldByName("result");
            message->GetReflection()->SetString(message, result_field, _result);
        }
    };
}