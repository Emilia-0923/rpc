#pragma once
#include "../concrete/ProtoResponse.hpp"

template<typename PbMessage>
class RpcResponse : public ProtoResponse<PbMessage>
{
public:
    using RpcResponse = std::shared_ptr<RpcResponse>;
};