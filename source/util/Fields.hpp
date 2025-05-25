#pragma once
#include <string>
#include <unordered_map>

namespace rpc
{
    namespace key {
        const std::string method = "method";
        const std::string params = "params";
        const std::string topic = "topic";
        const std::string message = "message";
        const std::string optype = "optype";
        const std::string address = "address";
        const std::string ip = "ip";
        const std::string port = "port";
        const std::string retcode = "retcode";
        const std::string result = "result";
    }

    enum class MsgType {
        REQ_RPC = 0, //请求RPC
        RSP_RPC = 1, //响应RPC
        REQ_TOPIC = 2, //请求主题
        RSP_TOPIC = 3, //响应主题
        REQ_SERVICE = 4, //请求服务
        RSP_SERVICE = 5, //响应服务
    };

    enum class RetCode {
        SUCCESS = 0, //成功
        PARSE_FAILED = 1, //解析失败
        INVALID_MSG = 2, //无效消息
        INVALID_PARAMS = 3, //无效参数
        DISCONNECTED = 4, //连接断开
        NOT_FOUND_SERVICE = 5, //未找到服务
        NOT_FOUND_TOPIC = 6, //未找到主题
    };

    static std::string err_reason(RetCode code) {
        static std::unordered_map<RetCode, std::string> err_map = {
            {RetCode::SUCCESS, "处理成功"},
            {RetCode::PARSE_FAILED, "解析失败"},
            {RetCode::INVALID_MSG, "无效消息"},
            {RetCode::INVALID_PARAMS, "无效参数"},
            {RetCode::DISCONNECTED, "连接断开"},
            {RetCode::NOT_FOUND_SERVICE, "未找到服务"},
            {RetCode::NOT_FOUND_TOPIC, "未找到主题"},
        };
        auto it = err_map.find(code);
        if (it != err_map.end()) {
            return it->second;
        }
        else {
            return "未知错误";
        }
    }

    // RPC请求类型
    enum class RpcType {
        SYNC = 0, //同步请求
        ASYNC = 1, //异步请求
        CALLBACK = 2, //回调请求
    };

    enum class TopicOptype {
        CREATE = 0, //创建主题
        REMOVE = 1, //删除主题
        SUBSCRIBE = 2, //订阅主题
        UNSUBSCRIBE = 3, //取消订阅主题
        PUBLISH = 4, //发布主题
    };

    enum class ServiceOptype {
        REGISTRY = 0, //注册服务
        DISCOVERY = 1, //发现服务
        ONLINE = 2, //上线服务
        OFFLINE = 3, //下线服务
    };
}