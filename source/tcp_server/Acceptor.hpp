#pragma once

#include "Socket.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"
#include "Channel.hpp"

class Acceptor
{
public:

    using acceptor_func = std::function<void(const Connection::Connection_Info&)>;

private:

    EventLoop* loop; //监听套接字事件监控
    Socket listen_socket; //监听套接字
    Channel listen_channel; //监听套接字事件管理

    acceptor_func accept_cb; //监听读事件回调函数
private:
    void handle_read() {
        std::string ip;
        uint16_t port;
        int newfd = listen_socket.accept(ip, port);
        if (newfd < 0) {
            logging.error("监听套接字与客户端建立连接时,发生错误!");
            return;
        }
        Connection::Connection_Info info = { newfd, ip, port };
        if (accept_cb) {
            accept_cb(info);
        }
    }

    Socket& create_server(uint16_t port) {
        listen_socket.create_server(Socket::IPV4_TCP, "0.0.0.0", port);
        return listen_socket;
    }

public:
    Acceptor(EventLoop* loop, int port)
        : loop(loop), listen_socket(create_server(port)), listen_channel(listen_socket.get_fd(), loop) {
        listen_channel.set_read_cb(std::bind(&Acceptor::handle_read, this));
    }

    void set_accept_cb(const acceptor_func& cb) {
        accept_cb = cb;
    }

    void listen() {
        logging.debug("监听套接字:%d, 启动了可读状态!", listen_socket.get_fd());
        listen_channel.enable_read();
    }
};