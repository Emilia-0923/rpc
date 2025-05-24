#pragma once
#include "ProtoMessage.hpp"

template <typename PbMessage>
class ProtoResponse : public ProtoMessage<PbMessage>
{
public:
    using ptr = std::shared_ptr<ProtoResponse>;

    virtual bool check() override {
        if(!message.has_rcode()) {
            logging.error("请求消息缺少返回码!");
            return false;
        }
        return true;
    }
};