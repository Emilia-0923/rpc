#pragma once

#include "Connector.hpp"
#include "../package/Connection.hpp"

namespace muduo {
    class TcpClient {
    private:
        using conn_func = Connection::conn_func;
        using msg_func = Connection::msg_func;
        using close_func = Connection::close_func;
        
        EventLoop* loop;
        std::shared_ptr<Connector> connector;
        std::string server_ip;
        uint16_t server_port;
        bool retry;
        bool is_connected;
        std::mutex mtx;
        Connection::ptr conn;
        conn_func conn_cb;
        msg_func msg_cb;
        close_func close_cb;

    public:

        TcpClient(EventLoop* _loop, const std::string& _ip, uint16_t _port)
            : loop(_loop)
            ,  connector(new Connector(_loop, _ip, _port))
            ,  server_ip(_ip)
            ,  server_port(_port)
            ,  retry(false)
            ,  is_connected(true) {
            connector->set_new_conn_cb(std::bind(&TcpClient::new_connection, this, std::placeholders::_1));
        }

        ~TcpClient() {
            disconnect();
        }

        void connect() {
            is_connected = true;
            connector->start();
        }

        void disconnect() {
            is_connected = false;
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (conn) {
                    conn->shutdown();
                }
            }
            connector->stop();
        }

        void enable_retry() {
            retry = true;
        }

        void set_conn_cb(conn_func cb) {
            conn_cb = cb;
        }

        void set_msg_cb(msg_func cb) {
            msg_cb = cb;
        }

        void set_close_callback(close_func cb) {
            close_cb = cb;
        }

        void stop() {
            is_connected = false;
            connector->stop();
        }

    private:
        void new_connection(int _sockfd) {
            loop->assert_in_loop();

            Connection::Info info;
            info.fd = _sockfd;
            info.ip = server_ip;
            info.port = server_port;

            Connection::ptr new_conn = std::make_shared<Connection>(loop, 0, info);
            new_conn->set_conn_cb(conn_cb);
            new_conn->set_msg_cb(msg_cb);
            new_conn->set_close_cb(close_cb);
            new_conn->set_svr_close_cb(std::bind(&TcpClient::remove_connection, this, std::placeholders::_1));
            {
                std::lock_guard<std::mutex> lock(mtx);
                conn = new_conn;
            }

            new_conn->established();
        }

        void remove_connection(const Connection::ptr& _conn) {
            loop->assert_in_loop();
            if(loop != conn->get_loop()) {
                logging.fatal("TcpClient::remove_connection loop != conn->get_loop()");
                abort();
            }
            {
                std::lock_guard<std::mutex> lock(mtx);
                if(conn != _conn) {
                    logging.fatal("TcpClient::remove_connection conn != _conn");
                    abort();
                }
                conn.reset();
            }
            loop->push_task(std::bind(&Connection::release, _conn));

            if (retry && is_connected) {
                logging.info("尝试重新连接 %s:%d", server_ip.c_str(), server_port);
                connector->restart();
            }
        }
    };
}