#pragma once

#include "Connector.hpp"
#include "../package/Connection.hpp"

namespace muduo {
    class TcpClient {
    private:
        EventLoop* loop;
        std::shared_ptr<Connector> connector;
        std::string server_ip;
        uint16_t server_port;
        bool retry;
        bool connected;
        std::mutex mutex;
        Connection::ptr conn;
        Connection::conn_func conn_cb;
        Connection::msg_func msg_cb;
        Connection::close_func close_cb;

        void new_connection(int sockfd) {
            loop->assert_in_loop();
            
            char buf[32];
            snprintf(buf, sizeof buf, ":%s:%d#%d", server_ip.c_str(), server_port, sockfd);
            std::string conn_name = buf;

            Connection::Info info;
            info.fd = sockfd;
            info.ip = server_ip;
            info.port = server_port;

            conn = std::make_shared<Connection>(loop, 0, info);
            conn->set_conn_cb(conn_cb);
            conn->set_msg_cb(msg_cb);
            conn->set_close_cb([this](Connection::ptr _conn) {
                loop->run_in_loop(std::bind(&TcpClient::remove_connection, this, _conn));
            });

            {
                std::lock_guard<std::mutex> lock(mutex);
                conn = conn;
            }

            conn->established();
        }

        void remove_connection(const Connection::ptr& _conn) {
            loop->assert_in_loop();
            {
                std::lock_guard<std::mutex> lock(mutex);
                conn.reset();
            }

            if (retry && connected) {
                logging.info("正在重连到 %s:%d", server_ip.c_str(), server_port);
                connector->restart();
            }
        }
    public:
        TcpClient(EventLoop* loop, const std::string& _ip, uint16_t _port)
            : loop(loop),
            connector(new Connector(loop, _ip, _port)),
            server_ip(_ip),
            server_port(_port),
            retry(false),
            connected(false) {
            connector->set_new_conn_cb(
                [this](int sockfd) { new_connection(sockfd); });
        }

        ~TcpClient() {
            disconnect();
        }

        void connect() {
            connected = true;
            connector->start();
        }

        void disconnect() {
            connected = false;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (conn) {
                    conn->shutdown();
                }
            }
            connector->stop();
        }

        void enable_retry() { retry = true; }

        void set_conn_cb(Connection::conn_func _cb) { conn_cb = _cb; }
        void set_msg_cb(Connection::msg_func _cb) { msg_cb = _cb; }
        void set_close_cb(Connection::close_func _cb) { close_cb = _cb; }
    };
}