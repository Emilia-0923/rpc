#pragma once

#include <mutex>
#include <condition_variable>

namespace muduo {
    class CountDownLatch {
    public:
        explicit CountDownLatch(int _count) : count(_count) {}

        // 阻塞当前线程直到计数器变为0
        void wait() {
            std::unique_lock<std::mutex> lock(mutex);
            while (count > 0) {
                cond.wait(lock);
            }
        }

        // 将计数器减1，并在计数器变为0时通知所有等待的线程
        void count_down() {
            std::lock_guard<std::mutex> lock(mutex);
            if (--count == 0) {
                cond.notify_all();
            }
        }

        // 返回当前计数器的值
        int get_count() const {
            std::lock_guard<std::mutex> lock(mutex);
            return count;
        }

    private:
        mutable std::mutex mutex;   // 使用mutable允许const成员函数中修改状态
        std::condition_variable cond;
        int count;
    };
}