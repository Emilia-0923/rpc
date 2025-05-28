#pragma once

#include <mutex>
#include <thread>
#include <sys/eventfd.h>
#include "Epoller.hpp"
#include "TimeWhell.hpp"

namespace muduo
{
    class EventLoop
    {
    private:
        using task_func = std::function<void()>;

        std::thread::id thread_id; //线程ID
        int event_fd; //eventfd唤醒IO事件监控有可能导致的阻塞
        Channel event_channel; //本loop的channel
        Epoller epoller; //描述符监控
        TimerWheel timer_whell; //定时器模块
        std::vector<task_func> tasks; //任务池
        std::mutex mtx;

    private:
        static int create_event_fd() {
            int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
            if (efd < 0) {
                logging.fatal("EventLoop::create_event_fd event_fd 创建错误!");
                abort();
            }
            logging.info("EventFd 创建成功, event_fd: %d!", efd);
            return efd;
        }

        //判断线程是否是EventLoop对应线程
        bool is_in_loop() {
            return thread_id == std::this_thread::get_id();
        }

        void execute_all_task() {
            std::vector<task_func> ready_tasks;
            //生命周期:准备任务时
            {
                std::unique_lock<std::mutex> lock(mtx);
                tasks.swap(ready_tasks);
            }
            for (auto& task : ready_tasks) {
                logging.debug("当前执行的任务: %p", task);
                task();
            }
        }

        void read_event_fd() {
            uint64_t res = 0;
            int ret = read(event_fd, &res, sizeof(res));
            if (ret < 0) {
                if (errno = EINTR || errno == EAGAIN) {
                    logging.warning("警告: %s", strerror(errno));
                    return;
                }
                else {
                    logging.fatal("EventLoop::read_event_fd 读取 event_fd: %d 出错!", event_fd);
                    abort();
                }
            }
        }

        //向event_fd中写入, 以唤醒epoll_wait, 防止任务队列中的任务因epoll_wait阻塞导致无法执行
        void awake_event_fd() {
            uint64_t val = 1;
            int ret = write(event_fd, &val, sizeof(val));
            if (ret < 0) {
                if (errno == EINTR) {
                    logging.warning("警告: %s", strerror(errno));
                }
            }
        }

    public:
        EventLoop()
            : thread_id(std::this_thread::get_id()), event_fd(create_event_fd()), event_channel(event_fd, this), timer_whell(this) {
            event_channel.set_read_cb(std::bind(&EventLoop::read_event_fd, this));
            event_channel.enable_read();
        }

        ~EventLoop() {
            logging.debug("EventLoop被释放: %p", this);
        }

        //事件监控->就绪事件处理->执行任务
        //1.获取到有新事件发生的Channel, 执行其中的回调
        //2.执行所有该线程的任务
        void start() {
            while (true) {
                std::vector<Channel*> actives;
                epoller.wait(actives);
                for (auto &active : actives)
                {
                    active->handler_event();
                }
                execute_all_task();
            }
        }

        bool has_timer(uint64_t _timer_id) {
            return timer_whell.has_timer(_timer_id);
        }

        //压入任务池
        void push_task(const task_func& _cb) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                tasks.emplace_back(_cb);
            }
            //唤醒可能因为没有事件就绪导致的epoll_wait阻塞
            //给eventfd写入数据，触发其可读事件, 让epoll_wait执行完毕
            awake_event_fd();
        }

        //判断任务是否处于当前线程中，是则执行，否则压入任务池
        void run_in_loop(const task_func& _cb) {
            if (is_in_loop()) {
                _cb();
            }
            else {
                push_task(_cb);
            }
        }

        void assert_in_loop() {
            if (thread_id != std::this_thread::get_id()) {
                logging.fatal("EventLoop::assert_in_loop 线程执行任务错误!");
                abort();
            }
        }

        //在epoller中添加/修改channel
        void epoll_update(Channel* _channel) {
            epoller.update(_channel);
        }

        //在epoller中删除channel
        void epoll_remove(Channel* _channel) {
            epoller.remove(_channel);
        }

        void timer_add(uint64_t _timer_id, uint32_t _timeout, const task_func& _task_cb) {
            timer_whell.add(_timer_id, _timeout, _task_cb);
        }

        void timer_refresh(uint64_t _timer_id) {
            timer_whell.refresh(_timer_id);
        }

        void timer_cancel(uint64_t _timer_id) {
            timer_whell.cancel(_timer_id);
        }
    };

    //在eventloop中更新Channel
    void Channel::update() {
        loop->epoll_update(this);
    }

    //在eventloop中删除Channel
    void Channel::remove() {
        logging.debug("尝试在 eventloop 中删除 fd: %d!", fd);
        loop->epoll_remove(this);
    }

    //添加定时器任务
    void TimerWheel::add(uint64_t _timer_id, uint32_t _timeout, const task_func& _task_cb) {
        loop->run_in_loop(std::bind(&TimerWheel::add_in_loop, this, _timer_id, _timeout, _task_cb));
    }

    //取消定时器任务
    void TimerWheel::cancel(uint64_t _timer_id) {
        loop->run_in_loop(std::bind(&TimerWheel::cancel_in_loop, this, _timer_id));
    }

    //刷新定时器任务
    void TimerWheel::refresh(uint64_t _timer_id) {
        loop->run_in_loop(std::bind(&TimerWheel::refresh_in_loop, this, _timer_id));
    }
}