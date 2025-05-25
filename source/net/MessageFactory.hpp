#pragma once
#include "../service/RpcRequst.hpp"
#include "../service/RpcResponse.hpp"
#include "../service/TopicRequest.hpp"
#include "../service/TopicResponse.hpp"
#include "../service/ServiceResponse.hpp"
#include "../service/ServiceResponse.hpp"

namespace rpc {
    class MessageFactory {
    public:
        template <typename PBMessage>
        static BaseMessage::ptr create(MsgType type) {
            switch (type)
            {
                case MsgType::REQ_RPC:
                    return RpcRequest<PBMessage>::ptr(new RpcRequest<PBMessage>());
                case MsgType::RSP_RPC:
                    return RpcResponse<PBMessage>::ptr(new RpcResponse<PBMessage>());
                case MsgType::REQ_TOPIC:
                    return TopicRequest<PBMessage>::ptr(new TopicRequest<PBMessage>());
                case MsgType::RSP_TOPIC:
                    return TopicResponse<PBMessage>::ptr(new TopicResponse<PBMessage>());
                case MsgType::REQ_SERVICE:
                    return ServiceRequest<PBMessage>::ptr(new ServiceRequest<PBMessage>());
                case MsgType::RSP_SERVICE:
                    return ServiceResponse<PBMessage>::ptr(new ServiceResponse<PBMessage>());
            }
        }

        template <typename PBMessage, typename ...Args>
        static std::shared_ptr<PBMessage> create(Args&&... args) {
            return std::make_shared<PBMessage>(std::forward(args)...);
        }
    };
}