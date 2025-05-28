#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include "../package/EventLoop.hpp"

namespace muduo
{
    class LoopThread
    {
    private:
        EventLoop* loop;
        std::thread loop_thread;

        std::mutex mtx;
        std::condition_variable cond;
        //通过条件变量和互斥锁,实现同步互斥的关系
    private:
        //实例化 EventLoop 对象，唤醒cond上有可能阻塞的线程，并且开始运行EventLoop模块的功能
        void thread_entry() {
            EventLoop this_loop;
            {
                std::unique_lock<std::mutex> lock(mtx);
                loop = &this_loop;
                cond.notify_all();
            }
            this_loop.start();
        }
    public:
        LoopThread()
            : loop(nullptr), loop_thread(std::thread(&LoopThread::thread_entry, this)) {
            loop_thread.detach();
        }

        EventLoop* get_loop() {
            EventLoop* this_loop = nullptr;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cond.wait(lock, [&]() {return loop != nullptr;});
                logging.debug("loop实例化完成: %p", loop);
                this_loop = loop;
            }
            return this_loop;
        }
    };
}