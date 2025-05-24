#pragma once
#include "LoopThread.hpp"

class LoopThreadPool
{
private:
    int thread_count;
    int next_idx;
    EventLoop* base_loop;
    std::vector<LoopThread*> threads;
    std::vector<EventLoop*> loops;
public:
    LoopThreadPool(EventLoop* _base_loop)
        : thread_count(0), next_idx(0), base_loop(_base_loop) {}

    void set_thread_count(int count) {
        thread_count = count;
    }

    void create() {
        if (thread_count > 0) {
            threads.resize(thread_count);
            loops.resize(thread_count);
            for (int i = 0; i < thread_count; i++) {
                threads[i] = new LoopThread();
                loops[i] = threads[i]->get_loop();
            }
        }
    }

    EventLoop* next_loop() {
        if (thread_count == 0) {
            return base_loop;
        }
        next_idx = (next_idx + 1) % thread_count;
        return loops[next_idx];
    }
};