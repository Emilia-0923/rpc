#pragma once

#include <iostream>
#include <functional>
#include <sys/epoll.h>
#include "../util/Log.hpp"

class Epoller;
class EventLoop;

class Channel
{
private:
    using func_t = std::function<void()>;

    int fd;
    EventLoop* loop;
    uint32_t event; //感兴趣的事件
    uint32_t revent; //已经发生了的事件
    func_t read_cb; //可读回调
    func_t write_cb; //可写回调
    func_t error_cb; //错误回调
    func_t close_cb; //断开连接回调
    func_t event_cb; //任意事件回调

private:
    void update();

public:

    Channel(int _fd, EventLoop* _loop)
        : fd(_fd), loop(_loop), event(0), revent(0) {
    }

    void remove();

    int get_fd() {
        return fd;
    }

    uint32_t get_event() {
        return event;
    }

    uint32_t get_revent() {
        return revent;
    }

    EventLoop* get_loop() {
        return loop;
    }

    //epoller检测到了事件，在此设置
    void set_event(uint32_t _event) {
        revent = _event;
    }

    void set_read_cb(const func_t& _cb) {
        //logging.debug("fd: %d, 被设置读事件回调: %p", fd, read_cb);
        read_cb = _cb;
    }

    void set_write_cb(const func_t& _cb) {
        //logging.debug("fd: %d, 被设置写事件回调: %p", fd, write_cb);
        write_cb = _cb;
    }

    void set_error_cb(const func_t& _cb) {
        //logging.debug("fd: %d, 被设置错误事件回调: %p", fd, error_cb);
        error_cb = _cb;
    }

    void set_close_cb(const func_t& _cb) {
        //logging.debug("fd: %d, 被设置断连事件回调: %p", fd, close_cb);
        close_cb = _cb;
    }

    void set_event_cb(const func_t& _cb) {
        //logging.debug("fd: %d, 被设置任意事件回调: %p", fd, event_cb);
        event_cb = _cb;
    }

    //可读
    bool read_able() {
        return event & EPOLLIN;
    }

    //可写
    bool write_able() {
        return event & EPOLLOUT;
    }

    //启用可读状态
    void enable_read() {
        logging.debug("fd: %d, 启用可读状态!", fd);
        event |= EPOLLIN;
        update();
    }

    //启用可写状态
    void enable_write() {
        logging.debug("fd: %d, 启用可写状态!", fd);
        event |= EPOLLOUT;
        update();
    }

    //关闭可读状态
    void disable_read() {
        logging.debug("fd: %d, 关闭可读状态!", fd);
        event &= ~EPOLLIN;
        update();
    }

    //关闭可写状态
    void disable_write() {
        logging.debug("fd: %d, 关闭可写状态!", fd);
        event &= ~EPOLLOUT;
        update();
    }

    void disable_all() {
        logging.debug("fd: %d, 清除了所有的状态!", fd);
        event = 0;
    }

    //执行回调函数    
    void handler_event() {
        // 可读 / 关闭连接 / 优先级事件
        if ((revent & EPOLLIN) || (revent & EPOLLRDHUP) || (revent & EPOLLPRI)) {
            if (read_cb) {
                read_cb();
            }
        }
        //可写
        if (revent & EPOLLOUT) {
            if (write_cb) {
                logging.debug("fd: %d, 调用了写事件回调函数: %p!", fd, write_cb);
                write_cb();
            }
        }
        //错误
        else if (revent & EPOLLERR) {
            if (error_cb) {
                logging.debug("fd: %d, 调用了错误事件回调函数: %p!", fd, error_cb);
                error_cb();
            }
        }
        //先读取完缓冲区的数据, 再关闭
        else if (revent & EPOLLHUP) {
            if (close_cb) {
                logging.debug("fd: %d, 调用了断连事件回调函数: %p!", fd, close_cb);
                close_cb();
            }
        }
        //常规事件
        if (event_cb) {
            event_cb();
        }
    }
};