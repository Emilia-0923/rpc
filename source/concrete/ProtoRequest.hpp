#pragma once
#include "ProtoMessage.hpp"

template <typename PbMessage>
class ProtoRequest : public ProtoMessage<PbMessage>
{
public:
    using ptr = std::shared_ptr<ProtoRequest>;
};