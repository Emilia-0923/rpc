#pragma once

#include "../muduo/package/Connection.hpp"
#include "../abstract/BaseProtocol.hpp"
#include "../abstract/BaseConnection.hpp"

namespace rpc {
    class MuduoConnection : public BaseConnection {
    private:
        BaseProtocol::ptr protocol;
        muduo::Connection::ptr conn;
    public:
        using ptr = std::shared_ptr<MuduoConnection>;

        MuduoConnection(const muduo::Connection::ptr conn, const BaseProtocol::ptr protocol) : conn(conn), protocol(protocol) {}

        virtual void send(const BaseMessage::ptr& message) {
            std::string body = protocol->serialize(message);
            conn->send(body.c_str(), body.size());
        }
        virtual void shutdown() {
            conn->shutdown();
        }
        virtual bool is_connected() {
            return conn->is_connected();
        }
    };
}