#pragma once

#include <iostream>
#include <cstdarg>
#include <string>
#include <ctime>
#include <mutex>
#include <unordered_map>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

class Log {
public:

    enum PRINT_STATUS {
        SCREEN,
        ONEFILE,
        CLASSFILE
    };

private:
    enum PRINT_STATUS print_method;
    std::string path;
    std::mutex log_mutex;

    const int buffer_size = 1024;

    std::string log_file;
    int log_level;

private:
    std::string level_to_string(int level) {
        switch (level) {
            case 0: return "Debug";
            case 1: return "Info";
            case 2: return "Warning";
            case 3: return "Error";
            case 4: return "Fatal";
            default: return "None";
        }
    }

    void printLog(const std::string& levelName, const std::string& logtxt) {
        switch (print_method) {
            case SCREEN:
                std::cout << logtxt;
                break;
            case ONEFILE:
                print_one_file(log_file, logtxt);
                break;
            case CLASSFILE:
                print_class_file(levelName, logtxt);
                break;
            default:
                break;
        }
    }

    void print_one_file(const std::string& logname, const std::string& logtxt) {
        std::lock_guard<std::mutex> lock(log_mutex);

        time_t t = time(nullptr);
        struct tm* ctime = localtime(&t);
        char buffer[buffer_size];
        snprintf(buffer, sizeof(buffer), "%02d-%02d-%02d_", ctime->tm_year + 1900, ctime->tm_mon + 1, ctime->tm_mday);
        std::string _logname = path + buffer;
        int fd = open(_logname.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd < 0)
            return;
        ssize_t ret = write(fd, logtxt.c_str(), logtxt.size());
        close(fd);
    }

    void print_class_file(const std::string& levelName, const std::string& logtxt) {
        std::string filename = log_file;
        filename += ".";
        filename += levelName;
        print_one_file(filename, logtxt);
    }

    void log_message(const std::string& levelName, const char* format, va_list args) {
        time_t t = time(nullptr);
        struct tm* ctime = localtime(&t);
        char leftbuffer[buffer_size];

        snprintf(leftbuffer, sizeof(leftbuffer), "[%s][thread: %p][%02d:%02d:%02d]", levelName.c_str(),
                 (void*)pthread_self(), ctime->tm_hour, ctime->tm_min, ctime->tm_sec);

        char rightbuffer[buffer_size];
        vsnprintf(rightbuffer, sizeof(rightbuffer), format, args);

        char logtxt[buffer_size * 2 + 1];
        snprintf(logtxt, sizeof(logtxt), "%s %s\n", leftbuffer, rightbuffer);

        std::lock_guard<std::mutex> lock(log_mutex); // 同样在这里加锁
        printLog(levelName, logtxt);
    }

public:
    Log() : print_method(SCREEN), log_file("log.txt"), log_level(0) {
        char* home = getenv("HOME");
        if (home != nullptr) {
            path = home;
            path += "/log/";
        } else {
            path = "./log/";
        }
    }

    void set_print_method(PRINT_STATUS method) {
        print_method = method;
    }

    void set_log_file(const std::string& logname) {
        log_file = logname;
    }

    void set_log_level(const std::string& level) {
        std::unordered_map<std::string, int> level_map = {
            {"debug", 0},
            {"info", 1},
            {"warning", 2},
            {"error", 3},
            {"fatal", 4}
        };
        log_level = level_map[level];
    }


    void debug(const char* format, ...) {
        if(log_level > 0) return; // 只在 log_level 为 0 时打印 Debug 日志
        va_list args;
        va_start(args, format);
        log_message("Debug", format, args);
        va_end(args);
    }

    void info(const char* format, ...) {
        if(log_level > 1) return; // 只在 log_level 为 1 时打印 Info 日志
        va_list args;
        va_start(args, format);
        log_message("Info", format, args);
        va_end(args);
    }

    void warning(const char* format, ...) {
        if(log_level > 2) return; // 只在 log_level 为 2 时打印 Warning 日志
        va_list args;
        va_start(args, format);
        log_message("Warning", format, args);
        va_end(args);
    }

    void error(const char* format, ...) {
        if(log_level > 3) return; // 只在 log_level 为 3 时打印 Error 日志
        va_list args;
        va_start(args, format);
        log_message("Error", format, args);
        va_end(args);
    }

    void fatal(const char* format, ...) {
        if(log_level > 4) return; // 只在 log_level 为 4 时打印 Fatal 日志
        va_list args;
        va_start(args, format);
        log_message("Fatal", format, args);
        va_end(args);
    }
};

Log logging;