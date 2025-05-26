#pragma once
#include "ProtoMessage.hpp"

namespace rpc
{
    class ProtoRequest : public ProtoMessage
    {
    public:
        using ptr = std::shared_ptr<ProtoRequest>;

        ProtoRequest(PBMessage* msg) : ProtoMessage(msg) {}
    };
}