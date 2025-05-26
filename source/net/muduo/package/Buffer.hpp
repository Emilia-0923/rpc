#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include "../util/Log.hpp"

namespace muduo
{
    class Buffer
    {
    private:
        static const size_t default_buffer_size = 65536;

        std::vector<char> buffer;
        size_t read_idx; //读偏移
        size_t write_idx; //写偏移
    private:
        char* begin() {
            return &*buffer.begin();
        }

        size_t head_space() {
            return read_idx;
        }

        size_t tail_space() {
            return buffer.size() - write_idx;
        }

        void write_check(size_t _len) {
            if (tail_space() >= _len) {
                return;
            }
            else if (head_space() + tail_space() >= _len) {
                //数据前移
                size_t read_size = read_able_size();
                std::copy(get_read_idx(), get_read_idx() + read_size, begin());
                read_idx = 0;
                write_idx = read_size;
            }
            else {
                //扩容
                logging.debug("buffer 扩容至: %d 字节", write_idx + _len);
                buffer.resize(write_idx + _len);
            }
        }
    public:

        Buffer()
            : buffer(default_buffer_size), read_idx(0), write_idx(0) {}

        Buffer(Buffer&& other) noexcept 
            : buffer(std::move(other.buffer)),
            read_idx(other.read_idx),
            write_idx(other.write_idx) {
            other.read_idx = other.write_idx = 0;
        }

        char* get_read_idx() {
            return begin() + read_idx;
        }

        char* get_write_idx() {
            return begin() + write_idx;
        }

        void move_write(size_t _len) {
            if (_len <= tail_space()) {
                write_idx += _len;
            }
            else {
                logging.error("缓冲区读取下标移动错误!");
            }
        }

        void move_read(size_t _len) {
            if (_len <= read_able_size()) {
                read_idx += _len;
            }
            else {
                logging.error("缓冲区写入下标移动错误!");
            }
        }

        size_t read_able_size() {
            return write_idx - read_idx;
        }

        void read(char* _buffer, size_t _len) {
            if (_len <= read_able_size()) {
                std::copy(get_read_idx(), get_read_idx() + _len, _buffer);
                move_read(_len);
            }
            else {
                logging.warning("读取数据大于可读空间大小!");
            }
        }

        void write(const char* _data, size_t _len) {
            write_check(_len);
            std::copy(_data, _data + _len, get_write_idx());
            move_write(_len);
        }

        void write_string(const std::string& _data) {
            write(_data.c_str(), _data.size());
        }

        void write_buffer(Buffer& _buffer) {
            write(_buffer.get_read_idx(), _buffer.read_able_size());
        }

        std::string read_string(size_t _len) {
            std::string str;
            str.resize(_len);
            read(&str[0], _len);
            return str;
        }

        std::string get_line() {
            const char* ptr = buffer.data() + read_idx;
            const char* pos = std::find(ptr, ptr + read_able_size(), '\n');

            if (pos != ptr + write_idx) {
                return read_string(pos - ptr + 1);
            }
            else {
                logging.warning("缓冲区读取失败, 行数不足一行!");
                return "";
            }
        }

        void clear() {
            read_idx = 0;
            write_idx = 0;
        }
    };
}