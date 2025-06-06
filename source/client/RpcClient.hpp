#pragma once

#include "../common/Dispatcher.hpp"
#include "RpcCaller.hpp"
#include "RpcRegistry.hpp"

namespace rpc {
    namespace client {
        // 注册服务客户端
        class RegistryClient {
        public:
            using ptr = std::shared_ptr<RegistryClient>;

            RegistryClient(const std::string& ip, int port)
                : requestor(std::make_shared<Requestor>())
                , provider(std::make_shared<Provider>(requestor))
                , dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_cb = std::bind(&Requestor::on_response, requestor, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<BaseMessage>(MsgType::RSP_SERVICE, rsp_cb);
                
                auto msg_cb = std::bind(&Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
                base_client = ClientFactory::create(ip, port);
                base_client->set_message_cb(msg_cb);
                base_client->connect();
            }

            bool registry_method(const std::string& method, const Address& host) {
                return provider->registry_method(base_client->get_connection(), method, host);
            }
        
        private:
            Requestor::ptr requestor;
            Provider::ptr provider;
            Dispatcher::ptr dispatcher;
            BaseClient::ptr base_client;
        };

        // 服务发现客户端
        class DiscoveryClient {
        public:
            using ptr = std::shared_ptr<DiscoveryClient>;

            DiscoveryClient(const std::string& ip, int port, const Discoverer::OfflineCallback& cb)
                : requestor(std::make_shared<Requestor>())
                , discoverer(std::make_shared<Discoverer>(requestor, cb))
                , dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_cb = std::bind(&Requestor::on_response, requestor, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<BaseMessage>(MsgType::RSP_SERVICE, rsp_cb);
                
                auto req_cb = std::bind(&Discoverer::on_service_request, discoverer, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<ServiceRequest>(MsgType::REQ_SERVICE, req_cb);

                auto msg_cb = std::bind(&Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
                base_client = ClientFactory::create(ip, port);
                base_client->set_message_cb(msg_cb);
                base_client->connect();
            }

            bool service_discovery(const std::string& method, Address& host) {
                return discoverer->service_discovery(base_client->get_connection(), method, host);
            }
                
        private:
            Requestor::ptr requestor;
            Discoverer::ptr discoverer;
            Dispatcher::ptr dispatcher;
            BaseClient::ptr base_client;
        };

        // RPC客户端
        class RpcClient {
        public:
            using ptr = std::shared_ptr<RpcClient>;
            using OfflineCallback = std::function<void(const Address&)>;

            RpcClient(bool _enable_discovery, const std::string& ip, int port)
                : enable_discovery(_enable_discovery)
                , requestor(std::make_shared<Requestor>())
                , caller(std::make_shared<RpcCaller>(requestor))
                , dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_cb = std::bind(&Requestor::on_response, requestor, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<BaseMessage>(MsgType::RSP_RPC, rsp_cb);

                if(enable_discovery) {
                    // 启用服务发现时创建一个服务发现客户端
                    discovery_client = std::make_shared<DiscoveryClient>(ip, port, std::bind(&RpcClient::remove_client, this, std::placeholders::_1));
                }
                else {
                    // 未启用服务发现时直接创建一个基础客户端
                    rpc_client = ClientFactory::create(ip, port);
                    auto msg_cb = std::bind(&Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
                    rpc_client->set_message_cb(msg_cb);
                    rpc_client->connect();
                }
            }

            bool sync_call(const std::string& method, const std::vector<PBValue>& param, PBValue& result) {
                BaseClient::ptr client = get_client(method);
                if(!client) {
                    logging.error("RpcClient::sync_call 获取客户端失败, method: %s", method.c_str());
                    return false;
                }
                logging.debug("RpcClient::sync_call 获取客户端成功, method: %s", method.c_str());
                return caller->sync_call(client->get_connection(), method, param, result);
            }

            bool async_call(const std::string& method, const std::vector<PBValue>& param, RpcCaller::PBAsyncResponse& result) {
                BaseClient::ptr client = get_client(method);
                if(!client) {
                    logging.error("RpcClient::async_call 获取客户端失败, method: %s", method.c_str());
                    return false;
                }
                return caller->async_call(client->get_connection(), method, param, result);
            }

            bool callback_call(const std::string& method, const std::vector<PBValue>& param, const RpcCaller::PBResponseCallback& cb) {
                BaseClient::ptr client = get_client(method);
                if(!client) {
                    logging.error("RpcClient::callback_call 获取客户端失败, method: %s", method.c_str());
                    return false;
                }
                return caller->callback_call(client->get_connection(), method, param, cb);
            }

        private:
            BaseClient::ptr create_client(const Address& host) {
                // 创建一个新的基础客户端
                BaseClient::ptr client = ClientFactory::create(host.first, host.second);
                if(!client) {
                    logging.error("RpcClient::create_client 创建客户端失败, host: {}", host.first);
                    return BaseClient::ptr();
                }
                auto msg_cb = std::bind(&Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
                client->set_message_cb(msg_cb);
                client->connect();
                add_client(host, client);
                return client;
            }

            BaseClient::ptr get_client(const Address& host) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = rpc_clients.find(host);
                if(it != rpc_clients.end()) {
                    return it->second;
                }
                return BaseClient::ptr();
            }

            BaseClient::ptr get_client(const std::string& method) {
                BaseClient::ptr client;
                if(enable_discovery) {
                    // 使用服务发现获取客户端
                    Address host;
                    if(discovery_client->service_discovery(method, host)) {
                        client = get_client(host);
                        if(!client) {
                            client = create_client(host);
                        }
                    }
                    else {
                        logging.error("RpcClient::get_client 服务发现失败, 没有找到服务提供者, method: %s", method.c_str());
                        return BaseClient::ptr();
                    }
                }
                else {
                    // 未启用服务发现时直接使用基础客户端
                    client = rpc_client;
                }
                return client;
            }

            void add_client(const Address& host, BaseClient::ptr client) {
                std::lock_guard<std::mutex> lock(mtx);
                rpc_clients[host] = client;
            }

            void remove_client(const Address& host) {
                std::lock_guard<std::mutex> lock(mtx);
                rpc_clients.erase(host);
            }

            struct AddressHash {
                size_t operator()(const Address& addr) const {
                    std::string host = addr.first + std::to_string(addr.second);
                    return std::hash<std::string>()(host);
                }
            };

            bool enable_discovery;
            Requestor::ptr requestor;
            RpcCaller::ptr caller;
            Dispatcher::ptr dispatcher;
            std::mutex mtx;
            DiscoveryClient::ptr discovery_client;
            BaseClient::ptr rpc_client; // 未启用服务发现时使用
            std::unordered_map<Address, BaseClient::ptr, AddressHash> rpc_clients; // 启用服务发现时使用
            OfflineCallback offline_callback; // 离线回调函数
        };
    }
}