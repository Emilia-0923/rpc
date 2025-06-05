#pragma once
#include "Requestor.hpp"
#include "../util/uuid.hpp"

namespace rpc {
    namespace client {
        // RPC服务提供者
        class Provider {
        public:
            using ptr = std::shared_ptr<Provider>;

            Provider(Requestor::ptr& _requestor) : requestor(_requestor) {}

            // 注册服务
            bool registry_method(const BaseConnection::ptr& _connection, const std::string& _method, const Address& _host) {
                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->set_id(UUID::ramdom());
                msg_req->set_type(MsgType::REQ_SERVICE);
                msg_req->set_method(_method);
                msg_req->set_address(_host);
                msg_req->set_optype(ServiceOptype::REGISTRY);
                BaseMessage::ptr base_rsp;
                if(!requestor->sync_send(_connection, msg_req, base_rsp)) {
                    logging.error("Provider::registry_method 发送请求失败");
                    return false;
                }
                auto msg_rsp = std::dynamic_pointer_cast<ServiceResponse>(base_rsp);
                if(!msg_rsp) {
                    logging.error("Provider::registry_method 接收响应失败");
                    return false;
                }
                if(msg_rsp->get_retcode() != RetCode::SUCCESS) {
                    logging.error("Provider::registry_method 注册失败, %s", err_reason(msg_rsp->get_retcode()).c_str());
                    return false;
                }
                return true;
            }
        
        private:
            Requestor::ptr requestor;
        };

        class MethodHost {
        public:
            using ptr = std::shared_ptr<MethodHost>;

            MethodHost(const std::vector<Address>& _hosts) : hosts(_hosts) {}

            // 添加host
            void append_host(const Address& _host) {
                std::lock_guard<std::mutex> lock(mtx);
                hosts.push_back(_host);
            }

            // 移除host
            void remove_host(const Address& _host) {
                std::lock_guard<std::mutex> lock(mtx);
                hosts.erase(std::remove(hosts.begin(), hosts.end(), _host), hosts.end());
            }

            // 获取下一个host
            Address get_host() {
                std::lock_guard<std::mutex> lock(mtx);
                size_t pos = index++ % hosts.size();
                return hosts[pos];
            }

            // 判断是否为空
            bool empty() {
                return hosts.empty();
            }
        private:
            std::mutex mtx;
            size_t index;
            std::vector<Address> hosts;
        };

        // RPC服务发现者
        class Discoverer {
        public:
            using ptr = std::shared_ptr<Discoverer>;
            using OfflineCallback = std::function<void(const Address&)>;

            Discoverer(const Requestor::ptr& _requestor, const OfflineCallback& _offline_callback)
                : requestor(_requestor), offline_callback(_offline_callback) {}

            // 服务发现
            bool service_discovery(const BaseConnection::ptr& _connection, const std::string& _method, Address& _host) {
                {
                    // 先从缓存中查找
                    std::lock_guard<std::mutex> lock(mtx);
                    auto it = method_hosts.find(_method);
                    if(it != method_hosts.end() && !it->second->empty()) {
                        _host = it->second->get_host();
                        return true;
                    }
                }
                // 当前没有可用的host
                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->set_id(UUID::ramdom());
                msg_req->set_type(MsgType::REQ_SERVICE);
                msg_req->set_method(_method);
                msg_req->set_optype(ServiceOptype::DISCOVERY);
                BaseMessage::ptr base_rsp;
                if(!requestor->sync_send(_connection, msg_req, base_rsp)) {
                    logging.error
                    ("Discoverer::service_discovery 发送请求失败");
                    return false;
                }
                auto msg_rsp = std::dynamic_pointer_cast<ServiceResponse>(base_rsp);
                if(!msg_rsp) {
                    logging.error("Discoverer::service_discovery 接收响应失败");
                    return false;
                }
                if(msg_rsp->get_retcode() != RetCode::SUCCESS) {
                    logging.error("Discoverer::service_discovery 发现服务失败, %s", err_reason(msg_rsp->get_retcode()).c_str());
                    return false;
                }
                std::lock_guard<std::mutex> lock(mtx);
                auto method_host = std::make_shared<MethodHost>(0, msg_rsp->get_address());
                if(method_hosts.empty()) {
                    logging.error("Discoverer::service_discovery 没有能够提供服务的节点, %s", msg_rsp->get_method().c_str());
                    return false;
                }
                _host = method_host->get_host();
                method_hosts[_method] = method_host;
                return true;
            }

            void on_service_request(const BaseConnection::ptr& _connection, const ServiceRequest::ptr& _req) {
                std::unique_lock<std::mutex> lock(mtx);
                if(_req->get_optype() == ServiceOptype::ONLINE) {
                    auto it = method_hosts.find(_req->get_method());
                    if(it != method_hosts.end()) {
                        it->second->append_host(_req->get_address());
                    }
                    else {
                        auto method_host = std::make_shared<MethodHost>(0, std::vector<Address>{_req->get_address()});
                        method_hosts[_req->get_method()] = method_host;
                    }
                }
                else if (_req->get_optype() == ServiceOptype::OFFLINE) {
                    auto it = method_hosts.find(_req->get_method());
                    if(it != method_hosts.end()) {
                        it->second->remove_host(_req->get_address());
                        offline_callback(_req->get_address());
                    }
                }
            }
        private:
            OfflineCallback offline_callback;
            std::mutex mtx;
            std::unordered_map<std::string, MethodHost::ptr> method_hosts;
            Requestor::ptr requestor;
        };
    }
}