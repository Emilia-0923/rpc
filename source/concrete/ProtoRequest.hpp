#pragma once
#include "ProtoMessage.hpp"
#include "../pbmessage/RpcRequest.pb.h"

namespace rpc
{
    class ProtoRequest : public ProtoMessage
    {
    public:
        using ptr = std::shared_ptr<ProtoRequest>;
    };
}