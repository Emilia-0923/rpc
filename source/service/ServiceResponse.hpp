#pragma once
#include "../concrete/ProtoResponse.hpp"

template<typename PbMessage>
class ServiceResponse : public ProtoResponse<PbMessage>
{
public:
    using ServiceRequestPtr = std::shared_ptr<ServiceRequest>;
};