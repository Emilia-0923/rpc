#pragma once

#include "../package/EventLoop.hpp"
#include "Channel.hpp"
#include "Socket.hpp"
#include "../util/Log.hpp"

namespace muduo {
    class Connector : public std::enable_shared_from_this<Connector> {
    public:
        using conn_cb = std::function<void(int sockfd)>;

        Connector(EventLoop* loop, const std::string& ip, uint16_t port)
            : loop(loop),
            server_ip(ip),
            server_port(port),
            connect(false),
            state(k_disconnected),
            socket_fd(-1),
            retry_delay_ms(init_retry_delay_ms) {}

        ~Connector() {
            if (socket_fd != -1) {
                ::close(socket_fd);
            }
        }

        void set_new_conn_cb(const conn_cb& _cb) {
            new_conn_cb = _cb;
        }

        void start() {
            connect = true;
            loop->run_in_loop(std::bind(&Connector::start_in_loop, this));
        }

        void restart() {
            retry_delay_ms = init_retry_delay_ms;
            start();
        }

        void stop() {
            connect = false;
            loop->run_in_loop(std::bind(&Connector::stop_in_loop, this));
        }

    private:
        enum State { k_disconnected, k_connecting, k_connected };
        static const int max_retry_delay_ms = 30000;
        static const int init_retry_delay_ms = 500;

        void set_state(State _state) { state = _state; }

        void start_in_loop() {
            loop->assert_in_loop();
            if (state != k_disconnected) return;

            if (connect) {
                connect_();
            }
        }

        void stop_in_loop() {
            loop->assert_in_loop();
            if (state == k_connecting) {
                state = k_disconnected;
                ::close(socket_fd);
                socket_fd = -1;
            }
        }

        void connect_() {
            socket_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
            if (socket_fd < 0) {
                logging.error("创建 socket 错误: %s", strerror(errno));
                retry();
                return;
            }

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);

            int ret = ::connect(socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            if (ret < 0 && errno != EINPROGRESS && errno != EINTR && errno != EISCONN) {
                logging.error("连接失败: %s", strerror(errno));
                ::close(socket_fd);
                socket_fd = -1;
                retry();
                return;
            }

            state = k_connecting;
            channel.reset(new Channel(socket_fd, loop));
            channel->set_write_cb(std::bind(&Connector::handle_write, this));
            channel->set_error_cb(std::bind(&Connector::handle_error, this));
            channel->enable_write();
        }

        void handle_write() {
            if (state != k_connecting) return;

            int err = 0;
            socklen_t len = sizeof(err);
            if (::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                logging.error("获取 socket 选项错误: %s", strerror(errno));
                err = errno;
            }

            if (err || is_self_connect(socket_fd)) {
                logging.warning("连接失败: %s", strerror(err));
                retry();
            } else {
                state = k_connected;
                if (connect) {
                    if (new_conn_cb) {
                        new_conn_cb(socket_fd);
                        socket_fd = -1; // 所有权已转移
                    }
                }
            }
        }

        void handle_error() {
            logging.warning("连接器发生错误");
            retry();
        }

        bool is_self_connect(int sockfd) {
            sockaddr_in local, peer;
            socklen_t len = sizeof(local);
            if (::getsockname(sockfd, (sockaddr*)&local, &len) < 0) return false;
            if (::getpeername(sockfd, (sockaddr*)&peer, &len) < 0) return false;
            return local.sin_port == peer.sin_port &&
                local.sin_addr.s_addr == peer.sin_addr.s_addr;
        }

        void retry() {
            ::close(socket_fd);
            socket_fd = -1;
            state = k_disconnected;
            if (connect) {
                logging.info("将在 %dms 后重试连接 %s:%d",
                            retry_delay_ms, server_ip.c_str(), server_port);
                uint64_t timer_id = reinterpret_cast<uint64_t>(this) + retry_delay_ms;
                loop->timer_add(timer_id, retry_delay_ms, std::bind(&Connector::start_in_loop, shared_from_this()));
                retry_delay_ms = std::min(retry_delay_ms * 2, max_retry_delay_ms);
            }
        }

        void reset_channel() {
            channel.reset();
        }

    private:
        EventLoop* loop;
        std::string server_ip;
        uint16_t server_port;
        bool connect;
        State state;
        int socket_fd;
        std::unique_ptr<Channel> channel;
        conn_cb new_conn_cb;
        int retry_delay_ms;
    };
}