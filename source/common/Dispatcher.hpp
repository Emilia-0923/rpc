#pragma once
#include "../net/factory/MessageFactory.hpp"
#include "../net/factory/ClientFactory.hpp"
#include "../net/factory/ServerFactory.hpp"

namespace rpc {
    class BaseCallBack {
    public:
        using ptr = std::shared_ptr<BaseCallBack>;
        virtual void on_message(const BaseConnection::ptr& conn, const BaseMessage::ptr& msg) = 0;
    };

    template<typename PBMessage>
    class CallBack : public BaseCallBack {
    public:
        using MessageCallBack = std::function<void(const BaseConnection::ptr&, std::shared_ptr<PBMessage>&)>;
        using ptr = std::shared_ptr<CallBack<PBMessage>>;

        CallBack(const MessageCallBack& _handler) : handler(_handler) { }

        virtual void on_message(const BaseConnection::ptr& conn, const BaseMessage::ptr& msg) override {
            auto pb_msg = std::dynamic_pointer_cast<PBMessage>(msg);
            handler(conn, pb_msg);
        }
    private:
        MessageCallBack handler;
    };

    class Dispatcher {
    private:
        std::mutex mtx;
        std::unordered_map<MsgType, BaseCallBack::ptr> handlers;

    public:
        using ptr = std::shared_ptr<Dispatcher>;
        template<typename PBMessage>
        void register_handler(MsgType type, const typename CallBack<PBMessage>::MessageCallBack& handler) {
            std::lock_guard<std::mutex> lock(mtx);
            auto cb = std::make_shared<CallBack<PBMessage>>(handler);
            handlers[type] = cb;
        }

        void on_message(const BaseConnection::ptr& conn, const BaseMessage::ptr& msg) {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = handlers.find(msg->get_type());
            if (it == handlers.end()) {
                logging.fatal("Dispatcher::on_message: 没有对应的处理函数, msg_type: %d", msg->get_type());
                conn->shutdown();
            }
            it->second->on_message(conn, msg);
        }
    };
}