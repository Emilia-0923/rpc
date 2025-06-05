#pragma once
#include "../net/abstract/BaseConnection.hpp"
#include "../net/factory/MessageFactory.hpp"

namespace rpc {
    namespace server {
        enum class ValueType {
            BOOL = 0,       // bool
            INTEGRAL = 1,   // int32_t
            NUMERIC = 2,    // double
            STRING = 3,     // std::string
            ARRAY = 4,      // std::vector
            OBJECT = 5,
        };

        using ServiceCallBack = std::function<void(const std::vector<PBValue>&, PBValue&)>;
        using ParamsDescribe = std::pair<std::string, ValueType>;

        // 服务描述类
        class ServiceDiscribe {
        public:
            using ptr = std::shared_ptr<ServiceDiscribe>;
            
            ServiceDiscribe(std::string&& _method_name, std::vector<ParamsDescribe>&& _params, ValueType&& _return_type, ServiceCallBack&& _callback)
            : method_name(std::move(_method_name))
            , params_desc(std::move(_params))
            , return_type(std::move(_return_type))
            , callback(std::move(_callback))
            {}

            const std::string& get_method_name() const {
                return method_name;
            }

            bool param_check(const std::vector<PBValue>& _params) {
                // 检查参数个数
                if (_params.size() != params_desc.size()) {
                    return false;
                }
                // 检查参数类型
                for (size_t i = 0; i < _params.size(); ++i) {
                    if (!check(params_desc[i].second, _params[i])) {
                        return false;
                    }
                }
                return true;
            }

            bool excute_callback(const std::vector<PBValue>& _params, PBValue& _result) {
                callback(_params, _result);
                if(!return_type_check(_result)) {
                    return false;
                }
                return true;
            }
        private:
            bool return_type_check(const PBValue& _value) {
                return check(return_type, _value);
            }

            bool check(ValueType _type, const PBValue& _value) {
                switch (_type) {
                    case ValueType::BOOL:
                        return _value.has_bool_value();
                    case ValueType::INTEGRAL:
                        return _value.has_number_value() && _value.number_value() == static_cast<int64_t>(_value.number_value());
                    case ValueType::NUMERIC:
                        return _value.has_number_value();
                    case ValueType::STRING:
                        return _value.has_string_value();
                    case ValueType::ARRAY:
                        return _value.has_list_value();
                    case ValueType::OBJECT:
                        return _value.has_struct_value();
                }
                return false;
            }

        private:
            std::string method_name;
            std::vector<ParamsDescribe> params_desc;
            ValueType return_type;
            ServiceCallBack callback; 
        };

        // 服务描述工厂类
        class ServiceDescribeFactory {
        public:
            void set_method_name(const std::string& _method_name) {
                method_name = _method_name;
            }

            void set_return_type(ValueType _type) {
                return_type = _type;
            }

            void set_param_desc(const std::string& _name, ValueType _type) {
                params_desc.emplace_back(_name, _type);
            }

            void set_callback(const ServiceCallBack& _callback) {
                callback = _callback;
            }

            ServiceDiscribe::ptr create() {
                return std::make_shared<ServiceDiscribe>(std::move(method_name), std::move(params_desc), std::move(return_type), std::move(callback));
            }
        private:
            std::string method_name;
            ServiceCallBack callback;
            std::vector<ParamsDescribe> params_desc;
            ValueType return_type;
        };

        // 服务管理器类
        class ServiceManager {
        public:
            using ptr = std::shared_ptr<ServiceManager>;

            void insert(const ServiceDiscribe::ptr& _desc) {
                std::lock_guard<std::mutex> lock(mtx);
                services[_desc->get_method_name()] = _desc;
            }

            ServiceDiscribe::ptr select(const std::string& _method_name) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = services.find(_method_name);
                if (it != services.end()) {
                    return it->second;
                }
                return nullptr;
            }

            void remove(const std::string& _method_name) {
                std::lock_guard<std::mutex> lock(mtx);
                services.erase(_method_name);
            }

        private:
            std::mutex mtx;
            std::unordered_map<std::string, ServiceDiscribe::ptr> services;
        };

        // RPC路由器类, 负责处理RPC请求和响应
        class RpcRouter {
        public:
            using ptr = std::shared_ptr<RpcRouter>;

            RpcRouter()
            : service_manager(std::make_shared<ServiceManager>()) {}

            void on_rpc_request(const BaseConnection::ptr& _conn, const RpcRequest::ptr& _req) {
                // 查询服务
                ServiceDiscribe::ptr service = service_manager->select(_req->get_method());
                if (!service.get()) {
                    logging.error("RpcRouter::on_rpc_request RPC方法不存在: %s", _req->get_method().c_str());
                    response(_conn, _req, PBValue(), RetCode::NOT_FOUND_SERVICE);
                    return;
                }
                // 检查参数
                if (!service->param_check(_req->get_params())) {
                    logging.error("RpcRouter::on_rpc_request RPC参数错误: %s", _req->get_method().c_str());
                    response(_conn, _req, PBValue(), RetCode::INVALID_PARAMS);
                    return;
                }
                // 调用回调
                PBValue result;
                if (!service->excute_callback(_req->get_params(), result)) {
                    logging.error("RpcRouter::on_rpc_request RPC回调执行失败: %s", _req->get_method().c_str());
                    response(_conn, _req, PBValue(), RetCode::INTERNAL_ERROR);
                    return;
                }
                // 返回结果
                response(_conn, _req, result, RetCode::SUCCESS);
            }

            void register_method(const ServiceDiscribe::ptr& _service) {
                service_manager->insert(_service);
            }

        private:
            void response(const BaseConnection::ptr& _conn, const RpcRequest::ptr& _req, const PBValue& _result, RetCode _retcode) {
                RpcResponse::ptr rsp = MessageFactory::create<RpcResponse>();
                rsp->set_id(_req->get_id());
                rsp->set_type(MsgType::RSP_RPC);
                rsp->set_retcode(_retcode);
                rsp->set_result(_result);
                _conn->send(rsp);
            }
        private:
            ServiceManager::ptr service_manager;
        };
    }
}