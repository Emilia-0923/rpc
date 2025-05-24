#pragma once

#include "NetWork.hpp"
#include "Connection.hpp"
#include "Acceptor.hpp"
#include "LoopThreadPool.hpp"

class TcpServer
{
private:
    uint16_t port;
    uint64_t id; //自增的连接ID
    int timeout; //非活跃连接统计时间
    bool inactive_release; //是否启用非活跃连接销毁
    EventLoop base_loop; //主线程的EventLoop,负责监听事件的处理
    Acceptor acceptor; //监听套接字的管理对象
    LoopThreadPool loop_pool; //EventLoop线程池
    std::unordered_map<uint64_t, Connection::Shared_Conn> connections;

    Connection::conn_func conn_cb;
    Connection::msg_func msg_cb;
    Connection::close_func close_cb;
    Connection::event_func event_cb;

private:
    void new_connection(const Connection::Connection_Info& info) {
        id++;
        Connection::Shared_Conn conn = std::make_shared<Connection>(loop_pool.next_loop(), id, info);
        conn->set_conn_cb(conn_cb);
        conn->set_msg_cb(msg_cb);
        conn->set_close_cb(close_cb);
        conn->set_event_cb(event_cb);
        conn->set_svr_close_cb(std::bind(&TcpServer::remove_connection, this, std::placeholders::_1));
        if (inactive_release == true) {
            conn->enable_inactive_release(timeout);
        }
        conn->established();//就绪初始化
        connections.emplace(id, conn);
    }

    void remove_connection_in_loop(const Connection::Shared_Conn& _conn) {
        uint64_t remove_id = _conn->get_id();
        auto it = connections.find(remove_id);
        if (it != connections.end()) {
            logging.debug("在TcpServer中移除了fd: %d 的连接.", _conn->get_fd());
            connections.erase(remove_id);
        }
        else {
            logging.warning("移除的连接不存在!");
        }
    }

    void remove_connection(const Connection::Shared_Conn& _conn) {
        base_loop.run_in_loop(std::bind(&TcpServer::remove_connection_in_loop, this, _conn));
    }

    void run_after_in_loop(const task_func& _task, int _delay) {
        id++;
        base_loop.timer_add(id, _delay, _task);
    }

public:
    TcpServer(uint16_t _port)
        : port(_port), id(0), inactive_release(false), acceptor(&base_loop, port), loop_pool(&base_loop) {
        acceptor.set_accept_cb(std::bind(&TcpServer::new_connection, this, std::placeholders::_1));
        acceptor.listen();
    }

    void set_thread_count(int count) {
        logging.info("设置LoopThreadPool线程池的线程数为: %d", count);
        loop_pool.set_thread_count(count);
    }

    void set_conn_cb(const Connection::conn_func& _cb) {
        conn_cb = _cb;
    }

    void set_msg_cb(const Connection::msg_func& _cb) {
        msg_cb = _cb;
    }

    void set_close_cb(const Connection::close_func& _cb) {
        close_cb = _cb;
    }

    void set_event_cb(const Connection::event_func& _cb) {
        event_cb = _cb;
    }

    void enable_inactive_release(int _timeout) {
        logging.info("设置TcpServer的超时关闭时间为: %d s", _timeout);
        timeout = _timeout;
        inactive_release = true;
    }

    //添加一个定时任务
    void run_after(const task_func& _task, int _delay) {
        base_loop.run_in_loop(std::bind(&TcpServer::run_after_in_loop, this, _task, _delay));
    }

    void start() {
        loop_pool.create();
        base_loop.start();
    }

};