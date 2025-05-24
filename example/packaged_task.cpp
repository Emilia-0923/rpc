#include <iostream>
#include <future>
#include <thread>

int Add(int a, int b) {
    std::cout << "执行异步任务" << std::endl; 
    return a + b;
}

int main() {
    // 封装异步任务
    auto task = std::make_shared<std::packaged_task<int(int, int)>>(Add);
    // 获取任务的future对象
    std::future<int> res = task->get_future();
    // 在新线程中执行任务
    std::thread([task]() {
        (*task)(1, 2);
    }).detach();
    // 获取结果
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "------------------------------" << std::endl;
    int num = res.get();
    std::cout << "The result is: " << num << std::endl;
    return 0;
}