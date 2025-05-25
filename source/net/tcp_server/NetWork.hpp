#pragma once
#include <signal.h>
#include "../util/Log.hpp"

namespace muduo
{
    class NetWork
    {
    public:
        NetWork() {
            //客户端断开连接，但服务器还在向客户端发数据,服务器会因为发送数据到已关闭的连接而收到 SIGPIPE,导致程序终止
            signal(SIGPIPE, SIG_IGN);
        }
    };
    static NetWork nw;
}