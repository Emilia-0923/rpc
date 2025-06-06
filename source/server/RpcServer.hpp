#pragma once

#include "../client/RpcClient.hpp"
#include "../common/Dispatcher.hpp"
#include "RpcRegistry.hpp"
#include "RpcRouter.hpp"

namespace rpc {
    namespace server {
        class RegistryServer {
        public:
            using ptr = std::shared_ptr<RegistryServer>;

            RegistryServer(const Address& host)
                : pd_manager(std::make_shared<PDManager>())
                , dispatcher(std::make_shared<Dispatcher>())
            {
                auto service_cb = std::bind(&PDManager::on_service_request, pd_manager, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<ServiceRequest>(MsgType::REQ_SERVICE, service_cb);

                server = ServerFactory::create(host.second, host.first);
                auto msg_cb = std::bind(&Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
                server->set_message_cb(msg_cb);
                auto close_cb = std::bind(&RegistryServer::on_connection_shutdown, this, std::placeholders::_1);
                server->set_close_cb(close_cb);
            }

            void start() {
                server->start();
            }
        private:
            void on_connection_shutdown(const BaseConnection::ptr& conn) {
                pd_manager->on_connection_shutdown(conn);
            }

            PDManager::ptr pd_manager;
            Dispatcher::ptr dispatcher;
            BaseServer::ptr server;
        };

        class RpcServer {
        public:
            using ptr = std::shared_ptr<RpcServer>;
 
            RpcServer(const Address& _host, bool _enable_registry = false, const Address& _registry_host = Address())
                : access_host(_host)
                , enable_registry(_enable_registry)
                , router(std::make_shared<RpcRouter>())
                , dispatcher(std::make_shared<Dispatcher>())
            {
                if (enable_registry) {
                    registry_client = std::make_shared<client::RegistryClient>(_registry_host.first, _registry_host.second);
                }

                auto rpc_cb = std::bind(&RpcRouter::on_rpc_request, router, std::placeholders::_1, std::placeholders::_2);
                dispatcher->register_handler<RpcRequest>(MsgType::REQ_RPC, rpc_cb);

                server = ServerFactory::create(access_host.second, access_host.first);
                auto msg_cb = std::bind(&Dispatcher::on_message, dispatcher, std::placeholders::_1, std::placeholders::_2);
                server->set_message_cb(msg_cb);
            }

            void register_method(const ServiceDiscribe::ptr& service_discribe) {
                if (enable_registry) {
                    logging.debug("RpcServer::register_method 向 %s:%d 注册了method: %s", access_host.first.c_str(), access_host.second, service_discribe->get_method_name().c_str());
                    registry_client->registry_method(service_discribe->get_method_name(), access_host);
                }
                router->register_method(service_discribe);
            }

            void start() {
                server->start();
            }
        private:
            Address access_host;
            bool enable_registry;
            client::RegistryClient::ptr registry_client;
            RpcRouter::ptr router;
            Dispatcher::ptr dispatcher;
            BaseServer::ptr server;
        };
    }
}