#pragma once
#include "concrete/ProtoResponse.hpp"

namespace rpc
{
    class RpcResponse : public ProtoResponse
    {
    public:
        using ptr = std::shared_ptr<RpcResponse>;

        RpcResponse() : ProtoResponse(new msg::RpcResponse()) {}

        virtual bool check() override {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* retcode_field = descriptor->FindFieldByName("retcode");
            const PBFieldDescriptor* result_field = descriptor->FindFieldByName("result");
            // 返回码字段存在，并且返回码类型为int32
            if (!retcode_field || retcode_field->cpp_type() != PBFieldDescriptor::CPPTYPE_INT32) {
                logging.error("RpcResponse 返回码为空或类型错误!");
                return false;
            }
            // 结果字段存在，并且结果类型为 google::protobuf::Value
            if (!result_field || result_field->message_type() != google::protobuf::Value::descriptor()) {
                logging.error("RpcResponse 返回结果为空或类型错误!");
                return false;
            }
            return true;
        }

        const PBValue& get_result() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* result_field = descriptor->FindFieldByName("result");
            const PBReflection* reflection = message->GetReflection();
            const auto& msg = message->GetReflection()->GetMessage(*message, result_field);
            return dynamic_cast<const google::protobuf::Value&>(msg);
        }

        void set_result(const PBValue _result) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* result_field = descriptor->FindFieldByName("result");
            message->GetReflection()->MutableMessage(message.get(), result_field)->CopyFrom(_result);
        }
    };
}