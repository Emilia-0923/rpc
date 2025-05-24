#pragma once
#include "BaseConnection.hpp"
#include <functional>

class BaseServer
{
private:
    using conn_func = std::function<void(const BaseConnection::ptr& conn)>;
    using close_func = std::function<void(const BaseConnection::ptr& conn)>;
    using msg_func = std::function<void(const BaseConnection::ptr& conn, const BaseMessage::ptr& msg)>;

    conn_func conn_cb;
    close_func close_cb;
    msg_func msg_cb;
public:
    using ptr = std::shared_ptr<BaseServer>;

    virtual void set_connected_cb(const conn_func& cb) {
        conn_cb = cb;
    }
    
    virtual void set_close_cb(const close_func& cb) {
        close_cb = cb;
    }
    
    virtual void set_message_cb(const msg_func& cb) {
        msg_cb = cb;
    }
    
    virtual void start() = 0;
};