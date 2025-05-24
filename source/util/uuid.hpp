#pragma once
#include <sstream>
#include <iomanip>
#include <random>
#include <atomic>

class UUID {
public:
     static std::string uuid() {
        std::stringstream ss;
        // 构造一个随机数生成器
        std::random_device rd;
        // 使用随机数生成器初始化一个均匀分布的整数生成器
        std::mt19937 generator(rd());
        // 生成8个随机数
        std::uniform_int_distribution<int> distribution(0, 255);
        for (int i = 0; i < 8; ++i) {
            // 生成一个随机数
            int random_number = distribution(generator);
            // 将随机数转换为16进制字符串并添加到字符串流中
            ss << std::hex << std::setw(2) << std::setfill('0') << random_number;
        }
        ss << '-';
        static std::atomic<size_t> id(1);
        size_t cur = id.fetch_add(1);
        // 将当前id转换为16进制字符串并添加到字符串流中
        for(int i = 7; i >= 0; i--) {
            if (i == 5) {
                ss << '-';
            }
            ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (i * 8)) & 0xFF);
        }
        return ss.str();
     }
};