#pragma once
#include "../concrete/ProtoRequest.hpp"

template<typename PbMessage>
class RpcRequest : public ProtoRequest<PbMessage>
{
public:
    using RpcRequestPtr = std::shared_ptr<RpcRequest>;

    // RPC请求: 方法名(string)和参数(vector<string>)
    virtual bool check() override {
        if(!message.has_method() || message.params_size() == 0) {
            logging.error("RpcRequest 方法名或参数为空!");
            return false;
        }
        return true;
    }

    std::string get_method() {
        return message.method();
    }

    void set_method(const std::string& _method) {
        message.set_method(_method);
    }

    std::vector<std::string> get_params() {
        std::vector<std::string> params;
        for(int i = 0; i < message.params_size(); i++) {
            params.push_back(message.params(i));
        }
        return params;
    }

    void set_params(const std::vector<std::string>& _params) {
        for(const auto& param : _params) {
            message.add_params(param);
        }
    }
};