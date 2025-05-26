#pragma once
#include "../../../abstract/BaseMessage.hpp"
#include "../../pbmessage/RpcMessage.pb.h"
#include "../../../util/Log.hpp"

namespace rpc
{
    using PBMessage = google::protobuf::Message;
    using PBDescriptor = google::protobuf::Descriptor;
    using PBFieldDescriptor = google::protobuf::FieldDescriptor;
    using PBReflection = google::protobuf::Reflection;

    class ProtoMessage : public BaseMessage
    {
    protected:
        // 消息体
        std::unique_ptr<PBMessage> message;
    public:
        using ptr = std::shared_ptr<ProtoMessage>;

        ProtoMessage(PBMessage* msg) : message(msg) {}
        
        virtual std::string serialize() override {
            // 序列化protobuf
            std::string str;
            if (message->SerializeToString(&str)) {
                return str;
            }
            else {
                logging.error("protobuf序列化失败!");
                return "";
            }
        }
        virtual bool deserialize(const std::string& msg) override {
            // 反序列化protobuf
            if (message->ParseFromString(msg)) {
                return true;
            }
            else {
                logging.error("protobuf反序列化失败!");
                return false;
            }
        }
    };
}