#pragma once

#include <cstring>
#include <sys/timerfd.h>
#include <memory>
#include <unordered_set>
#include "../util/Log.hpp"
#include "Channel.hpp"


using task_func = std::function<void()>;
using release_func = std::function<void()>;

class TimerTask
{
private:
    uint64_t timer_id; //定时器任务对象id
    uint32_t timeout; //定时任务的超时时间
    task_func task_cb; //任务回调函数
    release_func release_cb; //删除TimerWheel种保存的定时器对象信息
    bool valid; //是否有效

public:
    TimerTask(uint64_t _timer_id, uint32_t _timeout, const task_func& _task_cb)
        : timer_id(_timer_id), timeout(_timeout), task_cb(_task_cb), valid(true) {}

    //析构时执行任务
    ~TimerTask() {
        if (valid == true) {
            logging.debug("定时任务id: %d 被执行!", timer_id);
            task_cb();
        }
        release_cb();
    }

    void cancel() {
        valid = false;
    }

    uint64_t get_timer_id(){
        return timer_id;
    }

    uint32_t get_timeout() {
        return timeout;
    }

    //bind了remove_timer, 删除对应TimerWheel内统计的定时器任务
    void set_release(const release_func& _release_cb) {
        release_cb = _release_cb;
    }
};

class TimerWheel
{
private:
    using WeakTask = std::weak_ptr<TimerTask>;
    using SharedTask = std::shared_ptr<TimerTask>;


    EventLoop* loop;
    int timer_fd;
    Channel timer_channel;
    int capacity; //轮盘容量
    int tick; //秒针，走到哪释放哪
    std::vector<std::unordered_set<SharedTask>> wheel;
    std::unordered_map<uint64_t, WeakTask> timers;

private:

    //创建一个定时器文件描述符, 每秒超时一次(变得可读)
    //epoll监控该文件描述符, 超时被检测到, 执行对应回调函数
    //即可完成每秒执行对应的定时任务
    static int create_timer_fd() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0) {
            logging.fatal("timer_fd 创建错误: %s", strerror(errno));
            abort();
        }
        struct itimerspec itime;
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0;//第一次超时时间为1s后
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0; //第一次超时后，每次超时的间隔时
        timerfd_settime(timerfd, 0, &itime, NULL);
        logging.info("TimerFd 创建成功, timer_fd: %d!", timerfd);
        return timerfd;
    }

    uint64_t read_timer_fd() {
        uint64_t time;
        int ret = read(timer_fd, &time, sizeof(time));
        if (ret < 0) {
            logging.fatal("读取timer_fd失败: %s", strerror(errno));
            abort();
        }
        return time;
    }

    void add_in_loop(uint64_t _timer_id, uint32_t _timeout, const task_func& _task_cb) {
        SharedTask task_ptr(new TimerTask(_timer_id, _timeout, _task_cb));
        task_ptr->set_release(std::bind(&TimerWheel::remove_timer, this, _timer_id));
        int delay = task_ptr->get_timeout();
        logging.debug("添加了一个延迟为 %d 的定时任务, 任务id为 %ld!", delay, _timer_id);
        wheel[(tick + delay) % capacity].emplace(task_ptr);
        timers[_timer_id] = WeakTask(task_ptr);
    }

    void cancel_in_loop(uint64_t _timer_id) {
        auto it = timers.find(_timer_id);
        if (it == timers.end()) {
            logging.warning("取消定时任务失败,没有找到id: %ld 的定时任务!", _timer_id);
            return;
        }
        SharedTask task_ptr = it->second.lock();
        if (task_ptr) {
            task_ptr->cancel();
        }
    }

    void refresh_in_loop(uint64_t _timer_id) {
        //通过weakptr构造shared_ptr,添加到轮子中
        auto it = timers.find(_timer_id);
        if (it == timers.end()) {
            logging.warning("刷新定时任务失败,没有找到id: %ld 的定时任务!", _timer_id);
            return;
        }
        SharedTask task_ptr = it->second.lock();
        int delay = task_ptr->get_timeout();
        wheel[(tick + delay) % capacity].emplace(task_ptr);
    }

    //在timer中删除该任务，因为已经被执行过了
    void remove_timer(uint64_t _id) {
        auto it = timers.find(_id);
        if (it != timers.end()) {
            timers.erase(it);
        }
    }

    void run_task() {
        //每秒执行一次
        tick = (tick + 1) % capacity;
        if(wheel[tick].size() > 0){
            logging.debug("当前定时任务tick: %d, 当前定时任务轮盘的size: %d", tick, wheel[tick].size());
        }
        wheel[tick].clear(); // 清空指定位置的数组，通过析构函数自动调用任务
    }

    void on_time() {
        int time = read_timer_fd();
        for (int i = 0; i < time; i++) {
            run_task();
        }
    }

public:
    TimerWheel(EventLoop* _loop)
        : loop(_loop), timer_fd(create_timer_fd()), timer_channel(timer_fd, _loop), capacity(60), tick(0), wheel(capacity) {
        timer_channel.set_read_cb(std::bind(&TimerWheel::on_time, this));
        timer_channel.enable_read();
    }

    bool has_timer(uint64_t _timer_id) {
        auto it = timers.find(_timer_id);
        if (it == timers.end()) {
            return false;
        }
        else {
            return true;
        }
    }

    //添加定时器任务
    void add(uint64_t _timer_id, uint32_t _timeout, const task_func& _task_cb);

    //取消定时器任务
    void cancel(uint64_t _timer_id);

    //刷新定时器任务
    void refresh(uint64_t _timer_id);
};