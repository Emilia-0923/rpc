#pragma once
#include "../concrete/ProtoRequest.hpp"

template<typename PbMessage>
class TopicRequest : public ProtoRequest<PbMessage>
{
public:
    using TopicRequestPtr = std::shared_ptr<TopicRequest>;

    // 主题请求: 主题名称和操作类型
    virtual bool check() override {
        if(!message.has_topic() || !message.has_optype()) {
            logging.error("TopicRequest 主题名称或操作类型为空!");
            return false;
        }
        if(message.optype() == rpc::RequestType::PUBLISH) {
            if(!message.has_message()) {
                logging.error("TopicRequest 发布消息为空!");
                return false;
            }
        }
        return true;
    }

    std::string get_topic() {
        return message.topic();
    }

    void set_topic(const std::string& _topic) {
        message.set_topic(_topic);
    }

    rpc::TopicOptype get_optype() {
        return static_cast<rpc::TopicOptype>(message.optype());
    }

    void set_optype(rpc::TopicOptype _optype) {
        message.set_optype(_optype);
    }

    std::string get_message() {
        // 如果类型不为发布消息，则返回空字符串
        if(message.optype() != rpc::TopicOptype::PUBLISH) {
            return "";
        }
        return message.message();
    }

    void set_message(const std::string& _message) {
        // 如果类型不为发布消息，则返回空字符串
        if(message.optype() != rpc::TopicOptype::PUBLISH) {
            logging.error("TopicRequest 主题操作类型不为发布消息!");
            return;
        }
        message.set_message(_message);
    }
};