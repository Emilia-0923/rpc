#pragma once

#include "../util/Log.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

namespace muduo
{
    class Socket
    {
    public:

        enum Error
        {
            SocketErr = 1,
            BindErr = 2,
            ListenErr = 3,
        };

        enum Protocol
        {
            IPV4_TCP,
            IPV6_TCP,
        };

    private:

        struct SocketType
        {
            int domain;
            int type;
            int protocol;

            SocketType(){}

            SocketType(int domain, int type, int protocol)
                : domain(domain)
                , type(type)
                , protocol(protocol)
            {}
        };

        Protocol protocol;
        SocketType socket_type;
        int socket_fd;

        const int back_loging = 10;

    public:

        //domain(AF_INET/AF_INET6), type(SOCK_STREAM(TCP)/SOCK_DGRAM(UDP)), 0
        Socket()
            : socket_fd(-1), socket_type(-1, -1, -1){}

        Socket(int socket_fd, Protocol protocol)
            : socket_fd(socket_fd), protocol(protocol) {
            switch (protocol)
            {
            case IPV4_TCP:
                socket_type = SocketType(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                break;
            case IPV6_TCP:
                socket_type = SocketType(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                break;
            
            default:
                logging.fatal("协议参数错误!");
                abort();
            }
        }

        ~Socket() {
            remove();
        }
        
        void create(int domain, int type, int protocol) {
            socket_type.domain = domain;
            socket_type.type = type;
            socket_type.protocol = protocol;
            socket_fd = socket(socket_type.domain, socket_type.type, socket_type.protocol);
            if (socket_fd < 0)
            {
                logging.fatal("Socket 创建错误, %s: %d.", strerror(errno), errno);
                exit(Error::SocketErr);
            }
            logging.info("Socket 创建成功, socket_fd: %d!", socket_fd);
        }

        void remove(){
            if (socket_fd != -1) {
                logging.debug("关闭了套接字fd: %d", socket_fd);
                close(socket_fd);
                socket_fd = -1;
            }
        }

        void bind(const std::string& ip, const uint16_t& port)
        {
            if (socket_type.domain == AF_INET)
            {
                sockaddr_in local;
                memset(&local, 0, sizeof(local));
                local.sin_family = socket_type.domain;
                local.sin_port = htons(port);
                local.sin_addr.s_addr = inet_addr(ip.c_str());

                if (::bind(socket_fd, (sockaddr*)&local, sizeof(local)) < 0)
                {
                    logging.fatal("Socket 绑定端口错误, %s: %d.", strerror(errno), errno);
                    exit(Error::BindErr);
                }
                logging.info("Socket 绑定端口成功, port: %d!", port);
            }
        }

        void listen()
        {
            if (::listen(socket_fd, back_loging) < 0)
            {
                logging.fatal("Socket 监听错误, %s: %d.", strerror(errno), errno);
                exit(Error::ListenErr);
            }
            logging.info("Socket 监听成功!");
        }

        bool connect(const std::string& ip, const uint16_t& port)
        {
            sockaddr_in server;
            memset(&server, 0, sizeof(server));
            server.sin_family = socket_type.domain;
            server.sin_port = htons(port);
            inet_pton(socket_type.domain, ip.c_str(), &server.sin_addr);

            if (::connect(socket_fd, (const sockaddr*)&server, sizeof(server)))
            {
                logging.warning("Socket 连接错误, %s: %d.", strerror(errno), errno);
                return false;
            }
            logging.info("Socket 与服务器 %s:%d 建立连接成功!", ip.c_str(), port);
            return true;
        }

    public:


        //创建一个服务端连接
        void create_server(Protocol protocol, const std::string &ip, uint16_t port, bool block_flag = false) {
            //1. 创建套接字，2. 绑定地址，3. 开始监听，4. 设置非阻塞， 5. 启动地址重用
            switch (protocol)
            {
            case IPV4_TCP:
                create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                break;
            case IPV6_TCP:
                create(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                break;
            
            default:
                logging.fatal("协议参数错误!");
                abort();
            }
            if (block_flag) {
                non_block();
            }
            reuse_address();
            bind(ip, port);
            listen();
        }

        //创建一个客户端连接
        bool create_client(Protocol protocol, const std::string &ip, uint16_t port) {
            //1. 创建套接字，2.指向连接服务器
            switch (protocol)
            {
            case IPV4_TCP:
                create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                break;
            case IPV6_TCP:
                create(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                break;
            default:
                logging.fatal("协议参数错误!");
                abort();
            }
            return connect(ip, port);
        }

        //设置套接字选项---开启地址端口重用
        void reuse_address() {
            logging.debug("开启了地址端口重用");
            int opt = 1;
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
            //允许快速重启
        }
        
        //设置套接字阻塞属性-- 设置为非阻塞
        void non_block() {
            int flag = fcntl(socket_fd, F_GETFL, 0);
            fcntl(socket_fd, F_SETFL, flag | O_NONBLOCK);
        }

        int accept(std::string& client_ip, uint16_t& client_port)
        {
            sockaddr_in client;
            socklen_t size = sizeof(client);
            int client_fd = ::accept(socket_fd, (sockaddr*)&client, &size);
            if (client_fd < 0)
            {
                logging.warning("Socket 建立连接错误, %s: %d.", strerror(errno), errno);
                return -1;
            }

            char ip_str[64];
            client_port = ntohs(client.sin_port);
            inet_ntop(socket_type.domain, &client.sin_addr, ip_str, sizeof(ip_str));
            client_ip = ip_str;
            logging.info("Socket 与客户端 %s:%d 建立连接成功!", client_ip.c_str(), client_port);
            return client_fd;
        }

        ssize_t recv(void* buf, size_t len, int flag = 0) {
            ssize_t ret = ::recv(socket_fd, buf, len, flag);
            if (ret <= 0) {
                //EAGAIN 当前socket的接收缓冲区中没有数据了，在非阻塞的情况下才会有这个错误
                //EINTR  表示当前socket的阻塞等待，被信号打断了，
                if (errno == EAGAIN || errno == EINTR) {
                    return 0;
                }
                else if (errno == 0) {
                    logging.info("Socket_fd: %d, 关闭了连接: %s!", socket_fd, strerror(errno));
                    return -1;
                }
                else {
                    logging.error("Socket_fd: %d, 接收错误: %s!", socket_fd, strerror(errno));
                    return -1;
                }
            }
            return ret;
        }

        ssize_t non_block_recv(void* buf, size_t len) {
            if (len == 0) {
                return 0;
            }
            return recv(buf, len, MSG_DONTWAIT);
        }

        ssize_t send(const void* buf, size_t len, int flag = 0){
            ssize_t ret = ::send(socket_fd, buf, len, flag);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EINTR) {
                    return 0;
                }
                else {
                    logging.error("Socket_fd: %d, 发送错误!", socket_fd);
                    return -1;
                }
            }
            return ret;
        }

        ssize_t non_block_send(void* buf, size_t len) {
            if (len == 0) {
                return 0;
            }
            return send(buf, len, MSG_DONTWAIT);
        }

        int get_fd()
        {
            return socket_fd;
        }
    };
}