#pragma once
#include "../concrete/ProtoRequest.hpp"

template<typename PbMessage>
class ServiceRequest : public ProtoRequest<PbMessage>
{
public:
    using ServiceRequestPtr = std::shared_ptr<ServiceRequest>;
    using Address = std::pair<std::string, uint16_t>;

    // 服务请求: 服务名称和操作类型
    virtual bool check() override {
        if(!message.has_method() || !message.has_optype()) {
            logging.error("ServiceRequest 服务名称或操作类型为空!");
            return false;
        }
        if(!message.has_address()) {
            logging.error("ServiceRequest 地址信息为空!");
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

    rpc::ServiceOptype get_optype() {
        return static_cast<rpc::ServiceOptype>(message.optype());
    }

    void set_optype(rpc::ServiceOptype _optype) {
        message.set_optype(_optype);
    }
    
    Address get_address() {
        return { message.address().ip(), message.address().port() };
    }

    void set_address(const std::string& _ip, uint16_t _port) {
        auto address = message.mutable_address();
        address->set_ip(_ip);
        address->set_port(_port);
    }
};