#pragma
#include "../abstract/BaseMessage.hpp"

template <typename PbMessage>
class ProtoMessage : public BaseMessage
{
protected:
    // 消息体
    PbMessage message;
public:
    using ptr = std::shared_ptr<ProtoMessage>;
    
    virtual std::string serialize() override {
        // 序列化protobuf
        std::string str;
        if (message.SerializeToString(&str)) {
            return str;
        }
        else {
            logging.error("protobuf序列化失败!");
            return "";
        }
    }
    virtual bool deserialize(const std::string& msg) override {
        // 反序列化protobuf
        if (message.ParseFromString(msg)) {
            return true;
        }
        else {
            logging.error("protobuf反序列化失败!");
            return false;
        }
    }
};