#pragma once

#include "RpcRouter.hpp"
#include "../util/uuid.hpp"

namespace rpc {
    namespace server {
        // RPC服务注册中心
        class ProviderManager {
        public:
            using ptr = std::shared_ptr<ProviderManager>;

            struct Provider {
                using ptr = std::shared_ptr<Provider>;

                std::mutex mtx;
                BaseConnection::ptr connection;
                Address host;
                std::vector<std::string> methods;

                Provider(const BaseConnection::ptr& _connection, const Address& _host)
                    : connection(_connection), host(_host) {}

                void append_method(const std::string& _method) {
                    std::lock_guard<std::mutex> lock(mtx);
                    methods.push_back(_method);
                }
            };

            // 提供者注册服务
            void add_provider(const BaseConnection::ptr _connection, const Address& _host, const std::string& _method) {
                Provider::ptr provider;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    auto it = connection_provider.find(_connection);
                    if (it != connection_provider.end()) {
                        provider = it->second;
                    }
                    else {
                        provider = std::make_shared<Provider>(_connection, _host);
                        connection_provider[_connection] = provider;
                    }
                    method_provider[_method].insert(provider);
                }
                provider->append_method(_method);
            }

            // 提供者断连注销服务
            void del_provider(const BaseConnection::ptr _connection) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = connection_provider.find(_connection);
                if (it == connection_provider.end()) {
                    return;
                }
                for(auto& method : it->second->methods) {
                    method_provider[method].erase(it->second);
                }
                connection_provider.erase(it);
            }

            // 获取提供者注册的服务
            Provider::ptr get_provider(const BaseConnection::ptr _connection) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = connection_provider.find(_connection);
                if (it == connection_provider.end()) {
                    return Provider::ptr();
                }
                return it->second;
            }

            std::vector<Address> get_method_hosts(const std::string& _method) {
                std::lock_guard<std::mutex> lock(mtx);
                std::vector<Address> hosts;
                auto it = method_provider.find(_method);
                if (it == method_provider.end()) {
                    return hosts;
                }
                for(auto& provider : it->second) {
                    hosts.push_back(provider->host);
                }
                return hosts;
            }
        private:
            std::mutex mtx;
            std::unordered_map<std::string, std::unordered_set<Provider::ptr>> method_provider;
            std::unordered_map<BaseConnection::ptr, Provider::ptr> connection_provider;
        };

        // RPC服务发现中心
        class DiscovererManager {
        public:
            using ptr = std::shared_ptr<DiscovererManager>;

            struct Discoverer {
                using ptr = std::shared_ptr<Discoverer>;
                
                std::mutex mtx;
                BaseConnection::ptr connection;
                std::vector<std::string> methods;

                Discoverer(const BaseConnection::ptr& _connection)
                    : connection(_connection) {}

                void append_method(const std::string& _method) {
                    std::lock_guard<std::mutex> lock(mtx);
                    methods.push_back(_method);
                }
            };

            // 发现者注册服务
            Discoverer::ptr add_discoverer(const BaseConnection::ptr _connection, const std::string& _method) {
                Discoverer::ptr discoverer;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    auto it = connection_discoverer.find(_connection);
                    if (it == connection_discoverer.end()) {
                        discoverer = std::make_shared<Discoverer>(_connection);
                        connection_discoverer[_connection] = discoverer;
                    }
                    else {
                        discoverer = it->second;
                    }
                    method_discoverer[_method].insert(discoverer);
                }
                discoverer->append_method(_method);
                return discoverer;
            }

            // 发现者断连注销服务
            void del_discoverer(const BaseConnection::ptr _connection) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = connection_discoverer.find(_connection);
                if (it == connection_discoverer.end()) {
                    return;
                }
                for(auto& method : it->second->methods) {
                    method_discoverer[method].erase(it->second);
                }
                connection_discoverer.erase(it);
            }

            void online_notify(const std::string& _method, const Address& _host) {
                notify(_method, _host, ServiceOptype::ONLINE);   
            }

            void offline_notify(const std::string& _method, const Address& _host) {
                notify(_method, _host, ServiceOptype::OFFLINE);
            }
        private:
            void notify(const std::string& _method, const Address& _host, ServiceOptype _optype) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = method_discoverer.find(_method);
                if (it == method_discoverer.end()) {
                    return;
                }
                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->set_id(UUID::ramdom());
                msg_req->set_type(MsgType::REQ_SERVICE);
                msg_req->set_method(_method);
                msg_req->set_address(_host);
                msg_req->set_optype(_optype);
                for(auto& discoverer : it->second) {
                    discoverer->connection->send(msg_req);
                }
            }

            std::mutex mtx;
            std::unordered_map<std::string, std::unordered_set<Discoverer::ptr>> method_discoverer;
            std::unordered_map<BaseConnection::ptr, Discoverer::ptr> connection_discoverer;
        };

        // RPC服务注册中心管理器
        // 负责处理服务注册、发现和下线等操作
        class PDManager {
        public:
            using ptr = std::shared_ptr<PDManager>;

            PDManager()
                : provider_manager(std::make_shared<ProviderManager>()), discoverer_manager(std::make_shared<DiscovererManager>())
            {}

            void on_service_request(const BaseConnection::ptr _connection, const ServiceRequest::ptr _req) {
                // 先检查请求是否合法
                if (!_req->check()) {
                    logging.error("PDManager::on_service_request 请求格式错误: %s", _req->get_method().c_str());
                    return;
                }                
                ServiceOptype optype = _req->get_optype();
                if (optype == ServiceOptype::REGISTRY) {
                    logging.debug("PDManager::on_service_request: %s:%d 服务注册请求, method: %s", _req->get_address().first.c_str(), _req->get_address().second, _req->get_method().c_str());
                    provider_manager->add_provider(_connection, _req->get_address(), _req->get_method());
                    discoverer_manager->online_notify(_req->get_method(), _req->get_address());
                    registry_response(_connection, _req);
                }
                else if (optype == ServiceOptype::DISCOVERY) {
                    logging.debug("PDManager::on_service_request: %s:%d 服务发现请求, method: %s", _req->get_address().first.c_str(), _req->get_address().second, _req->get_method().c_str());
                    discoverer_manager->add_discoverer(_connection, _req->get_method());
                    discovery_response(_connection, _req);
                }
                else {
                    error_response(_connection, _req);
                    logging.error("PDManager::on_service_request: 错误的服务操作类型");
                }
            }

            void on_connection_shutdown(const BaseConnection::ptr _connection) {
                auto provider = provider_manager->get_provider(_connection);
                if (provider) {
                    for(auto& method : provider->methods) {
                        discoverer_manager->offline_notify(method, provider->host);
                    }
                    provider_manager->del_provider(_connection);
                }
                discoverer_manager->del_discoverer(_connection);
            }
        private:
            void registry_response(const BaseConnection::ptr _connection, const ServiceRequest::ptr _req) {
                auto msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->set_id(_req->get_id());
                msg_rsp->set_type(MsgType::RSP_SERVICE);
                msg_rsp->set_optype(ServiceOptype::REGISTRY);
                msg_rsp->set_retcode(RetCode::SUCCESS);
                _connection->send(msg_rsp);
            }

            void discovery_response(const BaseConnection::ptr _connection, const ServiceRequest::ptr _req) {
                auto msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->set_id(_req->get_id());
                msg_rsp->set_type(MsgType::RSP_SERVICE);
                msg_rsp->set_optype(ServiceOptype::DISCOVERY);
                std::vector<Address> hosts = provider_manager->get_method_hosts(_req->get_method());
                if(hosts.empty()) {
                    msg_rsp->set_retcode(RetCode::NOT_FOUND_SERVICE);
                    _connection->send(msg_rsp);
                    return;
                }
                msg_rsp->set_address(std::move(hosts));
                msg_rsp->set_method(_req->get_method());
                msg_rsp->set_retcode(RetCode::SUCCESS);
                _connection->send(msg_rsp);
            }

            void error_response(const BaseConnection::ptr _connection, const ServiceRequest::ptr _req) {
                auto msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->set_id(_req->get_id());
                msg_rsp->set_type(MsgType::RSP_SERVICE);
                msg_rsp->set_optype(ServiceOptype::SERVICE_UNKNOW);
                msg_rsp->set_retcode(RetCode::INVALID_OPTYPE);
                _connection->send(msg_rsp);
            }
            ProviderManager::ptr provider_manager;
            DiscovererManager::ptr discoverer_manager;
        };
    }
}