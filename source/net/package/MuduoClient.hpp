#pragma once

#include "../../abstract/BaseProtocol.hpp"
#include "../../abstract/BaseClient.hpp"
#include "../muduo/package/CountDownLatch.hpp"
#include "../muduo/tcp_client/TcpClient.hpp"
#include "../muduo/package/LoopThread.hpp"
#include "../factory/BufferFactory.hpp"
#include "../factory/ProtocolFactory.hpp"
#include "../factory/ConnectionFactory.hpp"

namespace rpc {
    class MuduoClient : public BaseClient {
    private:
        BaseProtocol::ptr protocol;
        BaseConnection::ptr conn;
        muduo::CountDownLatch latch;
        muduo::LoopThread loop_thread;
        muduo::EventLoop* base_loop;
        muduo::TcpClient client;

        static const int max_data_length = 1 << 16; // 64k

        void on_connected(const muduo::Connection::ptr& _conn) {
            if(_conn->is_connected()) {
                latch.count_down();
                conn = ConnectionFactory::create(_conn, protocol);
            }
            else {
                logging.info("连接断开!");
                conn.reset();
            }
        }

        void on_message(const muduo::Connection::ptr& _conn, muduo::Buffer* _buf) {
            auto base_buffer = BufferFactory::create(_buf);
            while(true) {
                if(!protocol->can_process(base_buffer)) {
                    logging.info("数据包不完整，等待数据包继续接收!"); 
                    if(base_buffer->read_able_size() > max_data_length) {
                        logging.info("数据包过大，关闭连接!"); 
                        _conn->shutdown();
                        return;
                    }
                    break;
                }
                BaseMessage::ptr base_message;
                if(!protocol->on_message(base_buffer, base_message)) {
                    logging.info("数据包解析失败，关闭连接!"); 
                    _conn->shutdown();
                    return;
                }
                if(msg_cb) {
                    msg_cb(conn, base_message);
                }
            }
        }


    public:
        using ptr = std::shared_ptr<MuduoClient>;

        MuduoClient(const std::string& ip, int port)
        : protocol(ProtocolFactory::create())
        , latch(1)
        , base_loop(loop_thread.get_loop())
        , client(base_loop, ip, port) {}

        virtual void connect() override {
            client.set_conn_cb(std::bind(&MuduoClient::on_connected, this, std::placeholders::_1));
            client.set_msg_cb(std::bind(&MuduoClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
            client.connect();
            latch.wait();
        }

        virtual void shutdown() override {
            client.disconnect();
        }

        virtual bool send(const BaseMessage::ptr& msg) {
            if(is_connected() == false) {
                logging.error("连接断开，发送失败!");
                return false;
            }
            conn->send(msg);
            return true;
        }

        virtual BaseConnection::ptr get_connection() {
            return conn;
        }

        virtual bool is_connected() {
            return conn && conn->is_connected();
        }
    };
}