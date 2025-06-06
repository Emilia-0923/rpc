#pragma once

#include "../abstract/BaseServer.hpp"
#include "../factory/BufferFactory.hpp"
#include "../factory/ConnectionFactory.hpp"
#include "../factory/ProtocolFactory.hpp" 
#include "../muduo/tcp_server/TcpServer.hpp"

namespace rpc {
    class MuduoServer : public BaseServer {
    private:
        BaseProtocol::ptr protocol;
        muduo::TcpServer server;
        std::mutex mtx;
        std::unordered_map<muduo::Connection::ptr, BaseConnection::ptr> connections;

        static const int max_data_length = 1 << 16; // 64k

        void on_connected(const muduo::Connection::ptr& conn) {
            if(conn->is_connected()) {
                logging.info("客户端连接成功!");
                auto muduo_conn = ConnectionFactory::create(conn, protocol);
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    connections[conn] = muduo_conn;
                }
                if(conn_cb) {
                    conn_cb(muduo_conn);
                }
            }
            else {
                logging.info("客户端断开连接!");
                auto muduo_conn = ConnectionFactory::create(conn, protocol);
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    auto it = connections.find(conn);
                    if(it != connections.end()) {
                        connections.erase(it);
                    }
                }
                if(close_cb) {
                    close_cb(muduo_conn);
                }
            }
        }

        void on_message(const muduo::Connection::ptr& conn, muduo::Buffer* buf) {
            auto base_buffer = BufferFactory::create(buf);
            while(true) {
                if(!protocol->can_process(base_buffer)) {
                    // logging.info("数据包不完整，等待数据包继续接收!"); 
                    if(base_buffer->read_able_size() > max_data_length) {
                        logging.info("数据包过大，关闭连接!"); 
                        conn->shutdown();
                        return;
                    }
                    break;
                }
                BaseMessage::ptr base_message;
                if(!protocol->on_message(base_buffer, base_message)) {
                    logging.info("数据包解析失败，关闭连接!"); 
                    conn->shutdown();
                    return;
                }
                BaseConnection::ptr base_conn;
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    auto it = connections.find(conn);
                    if(it == connections.end()) {
                        conn->shutdown();
                    }
                    base_conn = it->second;
                }
                if(msg_cb) {
                    msg_cb(base_conn, base_message);
                }
            }
        }
    public:
        using ptr = std::shared_ptr<MuduoServer>;

        MuduoServer(int port, const std::string& ip)
        : server(port, ip)
        , protocol(ProtocolFactory::create()) {}

        void start() {
            server.set_conn_cb(std::bind(&MuduoServer::on_connected, this, std::placeholders::_1));
            server.set_msg_cb(std::bind(&MuduoServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
            server.start();
        }
    }; 
}