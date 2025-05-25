#pragma once
#include "../util/Fields.hpp"
#include <memory>

namespace rpc
{
    class BaseMessage
    {
    private:
        rpc::MsgType type;
        std::string id;
    public:
        using ptr = std::shared_ptr<BaseMessage>;

        virtual ~BaseMessage() {}

        virtual std::string get_id() const {
            return id;
        }

        virtual void set_id(const std::string& id_) {
            id = id_;
        }

        virtual rpc::MsgType get_type() const {
            return type;
        }

        virtual void set_type(rpc::MsgType type_) {
            type = type_;
        }

        virtual std::string serialize() = 0;
        virtual bool deserialize(const std::string& msg) = 0;
    };
}