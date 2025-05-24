#include <iostream>
#include <future>
#include <thread>
#include <memory>

int Add(int a, int b) {
    std::cout << "执行异步任务" << std::endl; 
    return a + b;
}

int main() {
    // 实例化promise对象
    std::promise<int> promise;
    // 获取promise对象的future对象
    std::future<int> res = promise.get_future();
    // 给promise对象设置值
    std::thread([&promise]() {
        // 执行异步任务
        int result = Add(1, 2);
        // 设置promise对象的值
        promise.set_value(result);
    }).detach();
    // 等待结果
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "------------------------------" << std::endl;
    // 获取结果
    int num = res.get();
    std::cout << "The result is: " << num << std::endl;
    return 0;
}