#pragma once

#include "../../../util/Log.hpp"
#include "Channel.hpp"
#include <sys/epoll.h>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>

namespace muduo
{
    class Epoller
    {
    private:
        static const int size = 65535;
        static const int timeout = -1;

        int epoll_fd;
        std::vector<epoll_event> events;
        std::unordered_map<int, Channel*> channels;

    private:
        bool has_channel(Channel* _channel) {
            auto it = channels.find(_channel->get_fd());
            if (it == channels.end()) {
                return false;
            }
            return true;
        }
    public:
        Epoller()
            : epoll_fd(epoll_create(true)), events(size) {
            if (epoll_fd == -1) {
                logging.fatal("Epoller 创建失败: %s", strerror(errno));
            }
            else {
                logging.info("Epoller 创建成功, epoll_fd: %d!", epoll_fd);
            }
        }

        ~Epoller() {
            logging.info("~Epoller %d 被释放!", epoll_fd);
            if (epoll_fd >= 0) {
                close(epoll_fd);
            }
        }

        //添加/修改channel
        void update(Channel* _channel) {
            bool ret = has_channel(_channel);
            int fd = _channel->get_fd();
            if (fcntl(fd, F_GETFD) == -1) {
                logging.warning("fd: %d 已经被关闭，跳过 epoll 移除操作", fd);
                return;
            }
            epoll_event event;
            event.data.fd = fd;
            event.events = _channel->get_event();

            if (ret == false) {
                int n = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
                if (n == 0) {
                    channels.emplace(fd, _channel);
                }
            }
            else {
                int n = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
            }
        }

        //移除channel
        void remove(Channel* _channel) {
            int fd = _channel->get_fd();
            if (fcntl(fd, F_GETFD) == -1) {
                logging.warning("fd: %d 已经被关闭，跳过 epoll 移除操作", fd);
                return;
            }
            auto it = channels.find(fd);
            if (it != channels.end()) {
                channels.erase(it);
            }
            struct epoll_event event;
            event.data.fd = fd;
            event.events = _channel->get_event();
            int n = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
        }

        //开始检测IO事件
        //1.调用epoll_wait, 将发生的事件记录在epoller.events中
        //2.遍历events的fd, 在监控的channel中找到对应的fd, 并将epoller.events中发生的事件设置到revent中
        //3.把修改过的channel添加到active中, 以供Eventloop执行
        void wait(std::vector<Channel*>& _active, int _timeout = timeout) {
            int n = epoll_wait(epoll_fd, events.data(), events.size(), _timeout);
            if (n < 0) {
                if (errno == EINTR) {
                    logging.warning("警告: %s", strerror(errno));
                    return;
                }
                else {
                    logging.fatal("Epoller::wait 等待错误: %s", strerror(errno));
                    abort();
                }
            }
            for (int i = 0; i < n; i++) {
                auto it = channels.find(events[i].data.fd);
                if (it == channels.end()) {
                    logging.fatal("Epoller::wait 中等待的事件不存在!");
                    abort();
                }
                it->second->set_event(events[i].events);
                _active.emplace_back(it->second);
            }
        }

        int get_epoll_fd()
        {
            return epoll_fd;
        }
    };
}