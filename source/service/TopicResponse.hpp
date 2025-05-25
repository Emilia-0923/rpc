#pragma once
#include "../concrete/ProtoResponse.hpp"
#include "../pbmessage/RpcResponse.pb.h"

namespace rpc
{
    class TopicResponse : public ProtoResponse
    {
    public:
        using ptr = std::shared_ptr<TopicResponse>;
    };
}