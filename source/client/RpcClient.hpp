#pragma once

#include "../common/Dispatcher.hpp"
#include "RpcCaller.hpp"
#include "RpcRegistry.hpp"

namespace rpc {
    namespace client {
        class RegistryClient {
        public:
            using ptr = std::shared_ptr<RegistryClient>;

            RegistryClient(const std::string& ip, int port)
                : requestor(std::make_shared<Requestor>())
                , provider(std::make_shared<Provider>())
                , dispatcher(std::make_shared<Dispatcher>())
                , base_client(std::make_shared<BaseClient>())
            {
                auto rsp_cb = std::bind(Requestor::on_response, requestor, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<BaseMessage>(MsgType::RSP_SERVICE, rsp_cb);
                
                auto msg_cb = std::bind(Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
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

        class DiscoveryClient {
        public:
            using ptr = std::shared_ptr<DiscoveryClient>;

            DiscoveryClient(const std::string& ip, int port, const Discoverer::OfflineCallback& cb)
                : requestor(std::make_shared<Requestor>())
                , discoverer(std::make_shared<Discoverer>(requestor, cb))
                , dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_cb = std::bind(Requestor::on_response, requestor, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<BaseMessage>(MsgType::RSP_SERVICE, rsp_cb);
                
                auto req_cb = std::bind(Discoverer::on_service_request, discoverer, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<ServiceRequest>(MsgType::REQ_SERVICE, req_cb);

                auto msg_cb = std::bind(Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
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

        class RpcClient {
        public:
            using ptr = std::shared_ptr<RpcClient>;
        private:
            struct AddressHash {
                size_t operator()(const Address& addr) const {
                    std::string host = addr.first + std::to_string(addr.second);
                    return std::hash<std::string>()(host);
                }
            };

            DiscoveryClient::ptr discovery_client;
            RpcCaller::ptr caller;
            Requestor::ptr requestor;
            Dispatcher::ptr dispatcher;
            BaseClient::ptr base_client;
            std::mutex mtx;
            std::unordered_map<Address, BaseClient::ptr, AddressHash> rpc_clients;
        };
    }
}