#include "../source/net/factory/ClientFactory.hpp"
#include "../source/net/factory/ConnectionFactory.hpp"
#include "../source/net/factory/MessageFactory.hpp"
#include "../source/net/factory/BufferFactory.hpp"
#include "../source/net/factory/ProtocolFactory.hpp"
#include "../source/common/Dispatcher.hpp"
#include "../source/client/RpcCaller.hpp"
#include "../source/client/Requestor.hpp"

void callback(const rpc::PBValue& result) {
    std::cout << result.number_value() << std::endl;
}

int main() {
    auto requestor = std::make_shared<rpc::client::Requestor>();
    auto caller = std::make_shared<rpc::client::RpcCaller>(requestor);


    auto rsp_cb = std::bind(&rpc::client::Requestor::on_response , requestor.get(), std::placeholders::_1, std::placeholders::_2);
    auto dispatcer = std::make_shared<rpc::Dispatcher>();
    dispatcer->register_handler<rpc::RpcResponse>(rpc::MsgType::RSP_RPC, rsp_cb);

    auto client = rpc::ClientFactory::create("127.0.0.1", 9090);
    auto message_cb = std::bind(&rpc::Dispatcher::on_message, dispatcer.get(), std::placeholders::_1, std::placeholders::_2);
    client->set_message_cb(message_cb);
    client->connect();

    // auto conn = client->get_connection();
    // std::vector<rpc::PBValue> params;
    // rpc::PBValue param;
    // param.set_number_value(11);
    // params.push_back(param);
    // param.set_number_value(22);
    // params.push_back(param);
    // rpc::PBValue result;

    // bool ret = caller->call(conn, "Add", params, result);
    // if (ret) {
    //     std::cout << result.number_value() << std::endl;
    // }

    // rpc::client::RpcCaller::PBAsyncResponse res_future;
    // ret = caller->call(conn, "Add", params, res_future);
    // if (ret) {
    //     std::cout << res_future.get().number_value() << std::endl;
    // }

    // ret = caller->call(conn, "Add", params, callback);
    // if (ret) {
    //     std::cout << "call success" << std::endl;
    // }

    sleep(2);
    client->shutdown();
    return 0;
}