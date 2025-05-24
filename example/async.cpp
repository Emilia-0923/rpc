#include <iostream>
#include <future>
#include <thread>

int Add(int a, int b) {
    std::cout << "执行异步任务" << std::endl;
    return a + b;
}

int main() {
    std::future<int> res = std::async(std::launch::async, Add, 1, 2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "------------------------------" << std::endl;
    int num = res.get();
    std::cout << "The result is: " << num << std::endl;
    return 0;
}