#pragma once
#include "../concrete/ProtoResponse.hpp"

template<typename PbMessage>
class ServiceResponse : public ProtoResponse<PbMessage>
{
public:
    using TopicRequestPtr = std::shared_ptr<TopicRequest>;
};