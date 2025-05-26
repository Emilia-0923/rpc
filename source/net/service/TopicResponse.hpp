#pragma once
#include "concrete/ProtoResponse.hpp"

namespace rpc
{
    class TopicResponse : public ProtoResponse
    {
    public:
        using ptr = std::shared_ptr<TopicResponse>;

        TopicResponse() : ProtoResponse(new msg::TopicResponse()) {}
    };
}