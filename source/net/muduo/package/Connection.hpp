#pragma once

#include <memory>
#include <functional>
#include "Buffer.hpp"
#include "Any.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"

namespace muduo
{
    class Connection : public std::enable_shared_from_this<Connection> {
    public:

        struct Info
        {
            int fd;
            std::string ip;
            uint16_t port;
        };
        
        using ptr = std::shared_ptr<Connection>;
        using conn_func = std::function<void(ptr)>;
        using msg_func = std::function<void(ptr, Buffer*)>;
        using close_func = std::function<void(ptr)>;
        using event_func = std::function<void(ptr)>;

    private:

        static const int buffer_size = 65536;
        //DISCONNECTED 关闭状态
        //CONNECTING 连接建立完成,待处理状态
        //CONNECTED 连接可通信状态
        //DISCONNECTING 待关闭状态
        enum Status {
            DISCONNECTED,
            CONNECTING,
            CONNECTED,
            DISCONNECTING
        };

        Info info; //连接信息
        uint64_t id; //连接ID

        bool inactive_release; //是否启用非活跃销毁
        Status status; //连接状态
        // Socket socket; //套接字操作
        std::unique_ptr<Socket> socket; //套接字操作
        Channel conn_channel; //关联的Channel
        EventLoop* loop; //连接事件管理
        Buffer in_buffer; //输入缓冲区
        Buffer out_buffer; //输出缓冲区
        Any context; //接收的数据

        conn_func conn_cb;
        msg_func msg_cb;
        close_func close_cb;
        event_func event_cb;

        close_func server_close_cb; //组件内的连接关闭回调
        

    private:

        //设置到连接的channel中的读回调, 此时描述符应可读
        void handler_read() {
            //接收socket的数据，放到缓冲区
            char buf[buffer_size];
            int ret = socket->non_block_recv(buf, buffer_size - 1);
            if(ret < 0){
                shutdown();
                return;
            }
            in_buffer.write(buf, ret);
            if (in_buffer.read_able_size() > 0) {
                //调用message_callback进行业务处理
                msg_cb(shared_from_this(), &in_buffer);
            }
        }

        //设置到连接的channel中的写回调, 此时描述符应可写
        void handler_write() {
            ssize_t ret = socket->non_block_send(out_buffer.get_read_idx(), out_buffer.read_able_size());
            if (ret < 0) {
                if (in_buffer.read_able_size() > 0) {
                    msg_cb(shared_from_this(), &in_buffer);
                }
                release();
                return;
                //处理完接收缓冲区后关闭释放
            }
            //上面是直接在缓冲区中读取数据发送，没调用接口，所以要用move_read移动读指针
            out_buffer.move_read(ret);
            if (out_buffer.read_able_size() == 0) {
                conn_channel.disable_write();
                if (status == DISCONNECTING) {
                    release();
                }
            }

        }

        //设置到连接的channel中的关闭回调, 描述符断开时触发
        void handler_close() {
            if (in_buffer.read_able_size() > 0) {
                msg_cb(shared_from_this(), &in_buffer);
            }
            release();
        }

        //设置到连接的channel中的错误回调, 描述符出错时触发
        void handler_error() {
            handler_close();
        }

        //设置到连接的channel中的常规事件回调
        void handler_event() {
            if (inactive_release == true) {
                loop->timer_refresh(id);
            }
            if (event_cb) {
                event_cb(shared_from_this());
            }
        }

        //从半连接到完成连接状态
        void established_in_loop() {
            //修改连接状态
            if(status != CONNECTING)
            {
                logging.fatal("Connection::established_in_loop 错误! 连接状态不为预期状态");
                abort();
            }
            status = CONNECTED;
            conn_channel.enable_read();
            if (conn_cb) {
                conn_cb(shared_from_this());
            }
        }

        //释放接口
        void release_in_loop() {
            if (status != DISCONNECTING) {
                logging.warning("连接已经被释放，跳过重复释放操作");
                return;
            }
            status = DISCONNECTED;
            //取消事件关心
            conn_channel.disable_all();
            //移除连接的事件监控
            conn_channel.remove();
            //取消定时销毁任务
            if(loop->has_timer(id)) {
                disable_inactive_release_in_loop();
            }
            //调用关闭回调函数
            if (close_cb) {
                close_cb(shared_from_this());
            }
            if (server_close_cb) {
                server_close_cb(shared_from_this());
            }
        }
        
        //数据放到了发送缓冲区，启动可写事件监控
        void send_in_loop(Buffer& _buffer) {
            if (DISCONNECTED) {
                return;
            }
            // 读取前四个字节强转成uint32_t，先打印原始数据，再打印从网络字节序转换后的数据
            out_buffer.write_buffer(_buffer);
            if (conn_channel.write_able() == false) {
                conn_channel.enable_write();
            }
        }

        //loop中先将数据处理完毕, 再将连接关闭
        void shutdown_in_loop() {
            if (status != CONNECTED) {
                logging.warning("连接已经被释放，跳过重复释放操作");
                return;
            }
            status = DISCONNECTING;
            if (in_buffer.read_able_size() > 0) {
                if (msg_cb) {
                    msg_cb(shared_from_this(), &in_buffer);
                }
            }
            if (out_buffer.read_able_size() > 0) {
                if (conn_channel.write_able() == false) {
                    conn_channel.enable_write();
                }
            }
            if (out_buffer.read_able_size() == 0) {
                release_in_loop();
            }
        }

        void disconnect_in_loop() {
            if (status == DISCONNECTED) {
                logging.warning("连接已经被释放，跳过重复释放操作");
                return;
            }
            if (!conn_channel.write_able()) {
                ::shutdown(info.fd, SHUT_WR);
            }
        }

        void enable_inactive_release_in_loop(int _second) {
            inactive_release = true;
            if (loop->has_timer(id)) {
                loop->timer_refresh(id);
            }
            else {
                loop->timer_add(id, _second, std::bind(&Connection::release_in_loop, this));
            }
        }

        void disable_inactive_release_in_loop() {
            inactive_release = false;
            if (loop->has_timer(id)) {
                loop->timer_cancel(id);
            }
        }

        void upgrade_in_loop(const Any& _context, const conn_func& _conn_cb, const msg_func& _msg_cb, const close_func& _close_cb, const event_func& _event_cb) {
            set_context(_context);
            conn_cb = _conn_cb;
            msg_cb = _msg_cb;
            close_cb = _close_cb;
            event_cb = _event_cb;
        }
    public:
        Connection(EventLoop* _loop, uint64_t _id, const Connection::Info& _info)
            : info(_info), loop(_loop), id(_id), inactive_release(false), status(CONNECTING), socket(new Socket(info.fd, Socket::IPV4_TCP)), conn_channel(info.fd, _loop) {
            conn_channel.set_close_cb(std::bind(&Connection::handler_close, this));
            conn_channel.set_event_cb(std::bind(&Connection::handler_event, this));
            conn_channel.set_read_cb(std::bind(&Connection::handler_read, this));
            conn_channel.set_write_cb(std::bind(&Connection::handler_write, this));
            conn_channel.set_error_cb(std::bind(&Connection::handler_error, this));
        }

        ~Connection() {
            logging.debug("释放连接: %s:%d", info.ip.c_str(), info.port);
        }

        int get_fd(){
            return info.fd;
        }

        std::string get_ip(){
            return info.ip;
        }

        uint16_t get_port() {
            return info.port;
        }

        uint64_t get_id() {
            return id;
        }

        EventLoop* get_loop() {
            return loop;
        }

        bool is_connected() {
            return status == CONNECTED;
        }

        //设置协议格式
        void set_context(const Any& _context) {
            context = _context;
        }

        //获取协议格式
        Any& get_context() {
            return context;
        }

        void set_conn_cb(const conn_func& _cb) {
            //logging.debug("连接 %s:%d 被设置了连接回调函数: %p", info.ip.c_str(), info.port, _cb);
            conn_cb = _cb;
        }

        void set_msg_cb(const msg_func& _cb) {
            //logging.debug("连接 %s:%d 被设置了通信回调函数: %p", info.ip.c_str(), info.port, _cb);
            msg_cb = _cb;
        }

        void set_close_cb(const close_func& _cb) {
            //logging.debug("连接 %s:%d 被设置了断连回调函数: %p", info.ip.c_str(), info.port, _cb);
            close_cb = _cb;
        }

        void set_event_cb(const event_func& _cb) {
            //logging.debug("连接 %s:%d 被设置了常规事件回调函数: %p", info.ip.c_str(), info.port, _cb);
            event_cb = _cb;
        }

        void set_svr_close_cb(const close_func& _cb){
            server_close_cb = _cb;
        }

        //连接获取后，给状态进行初始化
        void established() {
            loop->run_in_loop(std::bind(&Connection::established_in_loop, this));
        }

        //发送数据,数据放到发送缓冲区，启动写事件监控
        void send(const char* _data, size_t _len) {
            Buffer buf;
            buf.write(_data, _len);
            loop->run_in_loop(std::bind(&Connection::send_in_loop, this, std::move(buf)));
        }

        //关闭连接
        void shutdown() {
            loop->run_in_loop(std::bind(&Connection::shutdown_in_loop, this));
        }

        void disconnect() {
            loop->run_in_loop(std::bind(&Connection::disconnect_in_loop, this));
        }

        //释放
        void release() {
            loop->push_task(std::bind(&Connection::release_in_loop, this));
        }

        //启用非活跃销毁
        void enable_inactive_release(int _second) {
            loop->run_in_loop(std::bind(&Connection::enable_inactive_release_in_loop, this, _second));
        }

        //关闭非活跃销毁
        void disable_inactive_release() {
            loop->run_in_loop(std::bind(&Connection::disable_inactive_release_in_loop, this));
        }

        //切换协议
        void upgrade(const Any& _context, const conn_func& _conn_cb, const msg_func& _msg_cb, const close_func& _close_cb, const event_func& _event_cb) {
            //必须在eventloop中立刻执行,防止新的事件触发后,切换任务还没被执行,导致使用原协议处理
            loop->assert_in_loop();
            loop->run_in_loop(std::bind(&Connection::upgrade_in_loop, this, _context, _conn_cb, _msg_cb, _close_cb, _event_cb));
        }
    };
}