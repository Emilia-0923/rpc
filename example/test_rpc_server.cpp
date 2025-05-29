#include "../source/server/RpcRouter.hpp"
#include "../source/common/Dispatcher.hpp"
#include "../source/net/factory/ServerFactory.hpp"
#include "../source/net/factory/ConnectionFactory.hpp"
#include "../source/net/factory/MessageFactory.hpp"
#include "../source/net/factory/BufferFactory.hpp"
#include "../source/net/factory/ProtocolFactory.hpp"

void Add(const std::vector<rpc::PBValue>& params, rpc::PBValue& result) {
    int num1 = params[0].number_value();
    int num2 = params[1].number_value();
    result.set_number_value(num1 + num2);
}

int main() {
    auto dispatcer = std::make_shared<rpc::Dispatcher>();
    std::unique_ptr<rpc::server::ServiceDescribeFactory> service_factory = std::make_unique<rpc::server::ServiceDescribeFactory>();
    service_factory->set_method_name("Add");
    service_factory->set_param_desc("num1", rpc::server::ValueType::INTEGRAL);
    service_factory->set_param_desc("num2", rpc::server::ValueType::INTEGRAL);
    service_factory->set_return_type(rpc::server::ValueType::INTEGRAL);
    service_factory->set_callback(Add);

    auto router = std::make_shared<rpc::server::RpcRouter>();
    router->register_method(service_factory->create());

    auto cb = std::bind(&rpc::server::RpcRouter::on_rpc_request, router.get(), std::placeholders::_1, std::placeholders::_2);
    dispatcer->register_handler<rpc::RpcRequest>(rpc::MsgType::REQ_RPC, cb);

    auto server = rpc::ServerFactory::create(9090);
    auto message_cb = std::bind(&rpc::Dispatcher::on_message, dispatcer.get(), std::placeholders::_1, std::placeholders::_2);
    server->set_message_cb(message_cb);
    server->start();
    return 0;
}