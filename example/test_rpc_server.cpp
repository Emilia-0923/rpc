#include "../source/common/Fields.hpp"
#include "../source/server/RpcServer.hpp"


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

    auto server = std::make_shared<rpc::server::RpcServer>(rpc::Address("127.0.0.1", 30001), true, rpc::Address("127.0.0.1", 30002));
    server->register_method(service_factory->create());
    server->start();
    return 0;
}