#pragma once

#include "RpcRouter.hpp"

namespace rpc {
    namespace server {
        class RpcRegistry {
        public:
            using ptr = std::shared_ptr<RpcRegistry>;

            struct Provider {
                using ptr = std::shared_ptr<Provider>;

                BaseConnection::ptr connection;
                Address host;
                std::vector<std::string> methods;

                Provider(const BaseConnection::ptr& _connection, const Address& _host)
                    : connection(_connection), host(_host) {}
            };

            // 提供者注册服务
            void add_provider(const BaseConnection::ptr _connection, const Address& _host, const std::string& _method) {
            }

            // 提供者断连注销服务
            void del_provider(const BaseConnection::ptr _connection) {
            }

            // 获取提供者注册的服务
            Provider::ptr get_provider(const BaseConnection::ptr _connection) {
            }

            std::vector<Address> get_method_hosts(const std::string& _method) {
            }
        private:
            std::mutex mtx;
            std::unordered_map<std::string, Provider::ptr> provider_map;
            std::unordered_map<BaseConnection::ptr, Provider::ptr> connection_map;
        };

        class Discoverer {
            
        };
    }
}