#pragma once
#include "BaseConnection.hpp"
#include <functional>

namespace rpc
{
    class BaseServer
    {
    protected:
        using ConnectedCallBack = std::function<void(const BaseConnection::ptr& conn)>;
        using CloseCallBack = std::function<void(const BaseConnection::ptr& conn)>;
        using MessageCallBack = std::function<void(const BaseConnection::ptr& conn, const BaseMessage::ptr& msg)>;

        ConnectedCallBack conn_cb;
        CloseCallBack close_cb;
        MessageCallBack msg_cb;
    public:
        using ptr = std::shared_ptr<BaseServer>;

        virtual void set_connected_cb(const ConnectedCallBack& cb) {
            conn_cb = cb;
        }
        
        virtual void set_close_cb(const CloseCallBack& cb) {
            close_cb = cb;
        }
        
        virtual void set_message_cb(const MessageCallBack& cb) {
            msg_cb = cb;
        }
        
        virtual void start() = 0;
    };
}