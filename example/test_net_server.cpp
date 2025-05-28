#include "../source/net/factory/ServerFactory.hpp"
#include "../source/net/factory/ConnectionFactory.hpp"
#include "../source/net/factory/MessageFactory.hpp"
#include "../source/net/factory/BufferFactory.hpp"
#include "../source/net/factory/ProtocolFactory.hpp"

void on_message(const rpc::BaseConnection::ptr& conn, const rpc::BaseMessage::ptr& msg) {
    std::string body = msg->serialize();
    std::cout << body << std::endl;
    std::cout << (int)msg->get_type() << std::endl;
    auto rsp = rpc::MessageFactory::create<rpc::RpcResponse>();
    rsp->set_id("id");
    rsp->set_type(rpc::MsgType::RSP_RPC);
    rsp->set_retcode(rpc::RetCode::SUCCESS);
    rsp->set_result("result");
    conn->send(rsp);
}

int main() {
    auto server = rpc::ServerFactory::create(9090);
    server->set_message_cb(on_message);
    server->start();
    return 0;
}