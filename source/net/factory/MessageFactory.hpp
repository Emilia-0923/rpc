#pragma once
#include "../service/RpcRequst.hpp"
#include "../service/RpcResponse.hpp"
#include "../service/TopicRequest.hpp"
#include "../service/TopicResponse.hpp"
#include "../service/ServiceRequest.hpp"
#include "../service/ServiceResponse.hpp"

namespace rpc {
    class MessageFactory {
    public:
        static BaseMessage::ptr create(MsgType type) {
            switch (type)
            {
                case MsgType::REQ_RPC:
                    return RpcRequest::ptr(new RpcRequest());
                case MsgType::RSP_RPC:
                    return RpcResponse::ptr(new RpcResponse());
                case MsgType::REQ_TOPIC:
                    return TopicRequest::ptr(new TopicRequest());
                case MsgType::RSP_TOPIC:
                    return TopicResponse::ptr(new TopicResponse());
                case MsgType::REQ_SERVICE:
                    return ServiceRequest::ptr(new ServiceRequest());
                case MsgType::RSP_SERVICE:
                    return ServiceResponse::ptr(new ServiceResponse());
            }
            return BaseMessage::ptr();
        }

        template <typename PBMessage, typename ...Args>
        static std::shared_ptr<PBMessage> create(Args&&... args) {
            return std::make_shared<PBMessage>(std::forward(args)...);
        }
    };
}