#pragma once

#include "../package/EventLoop.hpp"
#include "../package/Channel.hpp"
#include "../package/Socket.hpp"
#include "../../../util/Log.hpp"

namespace muduo {
    class Connector : public std::enable_shared_from_this<Connector> {
    private:
        enum State { DISCONNECTED, CONNECTING, CONNECTED };
        const int max_retry_delay = 8;
        const int init_retry_delay = 1;
        uint64_t timer_id = reinterpret_cast<uint64_t>(this);

        using conn_cb = std::function<void(int sockfd)>;

        EventLoop* loop;
        std::string server_ip;
        uint16_t server_port;
        bool is_connect;
        State state;
        int socket_fd;
        std::unique_ptr<Channel> channel;
        conn_cb new_conn_cb;
        int retry_delay;
        
    public:
        using ptr = std::shared_ptr<Connector>;

        Connector(EventLoop* loop, const std::string& ip, uint16_t port)
            : loop(loop),
            server_ip(ip),
            server_port(port),
            is_connect(false),
            state(DISCONNECTED),
            retry_delay(init_retry_delay) {}

        ~Connector() {
            if(channel) {
                logging.fatal("~Connector channel 未知错误");
                abort();
            }
        }

        void set_new_conn_cb(const conn_cb& _cb) {
            new_conn_cb = _cb;
        }

        void start() {
            is_connect = true;
            loop->run_in_loop(std::bind(&Connector::start_in_loop, shared_from_this()));
        }

        void restart() {
            loop->assert_in_loop();
            state = DISCONNECTED;
            retry_delay = init_retry_delay;
            is_connect = true;
            start_in_loop();
        }

        void stop() {
            is_connect = false;
            loop->push_task(std::bind(&Connector::stop_in_loop, shared_from_this()));
            if(loop->has_timer(timer_id)) {
                loop->timer_cancel(timer_id);
            }
        }

    private:

        void set_state(State _state) { state = _state; }

        void start_in_loop() {
            loop->assert_in_loop();
            if (state != DISCONNECTED) {
                logging.fatal("Connector::start_in_loop 状态错误: %d", state);
            }

            if (is_connect) {
                connect();
            }
            else {
                logging.info("未开启连接");
            }
        }

        void stop_in_loop() {
            loop->assert_in_loop();
            if (state == CONNECTING) {
                state = DISCONNECTED;
                int socket_fd = remove_and_reset_channel();
                retry(socket_fd);
            }
        }

        void connect() {
            int socket_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);
            int ret = ::connect(socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            int err = ret == 0 ? 0 : errno;
            switch (err)//错误处理
            {
                case 0: case EINPROGRESS: case EINTR: case EISCONN:
                connecting(socket_fd);
                break;
                case EAGAIN: case EADDRINUSE: case EADDRNOTAVAIL: case ECONNREFUSED: case ENETUNREACH:
                retry(socket_fd);
                break;
                case EACCES: case EPERM: case EAFNOSUPPORT: case EALREADY: case EBADF: case EFAULT: case ENOTSOCK:
                ::close(socket_fd);
                break;
                default:
                ::close(socket_fd);
                break;
            }
        }

        void connecting(int socket_fd) {
            state = CONNECTING;
            if(channel) {
                logging.fatal("Connector::connecting channel 未知错误");
                abort();
            }
            channel.reset(new Channel(socket_fd, loop));
            channel->set_write_cb(std::bind(&Connector::handle_write, shared_from_this()));
            channel->set_error_cb(std::bind(&Connector::handle_error, shared_from_this()));
            channel->enable_write();
        }

        void handle_write() {
            if (state != CONNECTING) {
                logging.fatal("Connector::handle_writes 状态错误: %d", state);
                abort();
            }
            int socket_fd = remove_and_reset_channel();
            int err = 0;
            socklen_t len = sizeof(err);
            if (::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                logging.error("获取 socket 选项错误: %s", strerror(errno));
                err = errno;
            }

            if (err || is_self_connect(socket_fd)) {
                logging.warning("连接失败: %s", strerror(err));
                retry(socket_fd);
            }
            else {
                state = CONNECTED;
                if (is_connect) {
                    new_conn_cb(socket_fd);
                }
                else {
                    logging.info("未开启连接");
                    // 关闭socket
                    ::close(socket_fd);
                }
            }
        }

        void handle_error() {
            if(state == CONNECTING) {
                int socket_fd = remove_and_reset_channel();
                int err = 0;
                socklen_t len = sizeof(err);
                if (::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                    logging.error("获取 socket 选项错误: %s", strerror(errno));
                    err = errno;
                }
                logging.warning("连接失败: %s", strerror(err));
                retry(socket_fd);
            }
        }

        bool is_self_connect(int sockfd) {
            sockaddr_in local, peer;
            socklen_t len = sizeof(local);
            if (::getsockname(sockfd, (sockaddr*)&local, &len) < 0) return false;
            if (::getpeername(sockfd, (sockaddr*)&peer, &len) < 0) return false;
            return local.sin_port == peer.sin_port &&
                local.sin_addr.s_addr == peer.sin_addr.s_addr;
        }

        void retry(int socket_fd) {
            ::close(socket_fd);
            state = DISCONNECTED;
            if (is_connect) {
                logging.info("将在 %ds 后重试连接 %s:%d",
                            retry_delay, server_ip.c_str(), server_port);
                loop->timer_add(timer_id, retry_delay, std::bind(&Connector::start_in_loop, shared_from_this()));
                retry_delay = std::min(retry_delay * 2, max_retry_delay);
            }
        }

        void reset_channel() {
            channel.reset();
        }

        int remove_and_reset_channel() {
            channel->disable_all();
            channel->remove();
            int socket_fd = channel->get_fd();
            loop->push_task(std::bind(&Connector::reset_channel, shared_from_this()));
            return socket_fd;
        }
    };
}