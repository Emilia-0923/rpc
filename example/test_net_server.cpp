#include "../source/net/factory/ServerFactory.hpp"
#include "../source/net/factory/ConnectionFactory.hpp"
#include "../source/net/factory/MessageFactory.hpp"
#include "../source/net/factory/BufferFactory.hpp"
#include "../source/net/factory/ProtocolFactory.hpp"
#include "../source/dispatcher/Dispatcher.hpp"


void on_rpc_request(const rpc::BaseConnection::ptr& conn, const rpc::BaseMessage::ptr& msg) {
    std::string body = msg->serialize();
    std::cout << body << std::endl;
    auto rsp = rpc::MessageFactory::create<rpc::RpcResponse>();
    rsp->set_id("id");
    rsp->set_type(rpc::MsgType::RSP_RPC);
    rsp->set_retcode(rpc::RetCode::SUCCESS);
    rsp->set_result("result");
    conn->send(rsp);
}

void on_topic_request(const rpc::BaseConnection::ptr& conn, const rpc::BaseMessage::ptr& msg) {
    std::string body = msg->serialize();
    std::cout << body << std::endl;
    auto rsp = rpc::MessageFactory::create<rpc::TopicResponse>();
    rsp->set_id("id");
    rsp->set_type(rpc::MsgType::RSP_TOPIC);
    rsp->set_retcode(rpc::RetCode::SUCCESS);
    conn->send(rsp);
}

// void on_message(const rpc::BaseConnection::ptr& conn, const rpc::BaseMessage::ptr& msg) {
//     std::string body = msg->serialize();
//     std::cout << body << std::endl;
//     std::cout << (int)msg->get_type() << std::endl;
//     auto rsp = rpc::MessageFactory::create<rpc::RpcResponse>();
//     rsp->set_id("id");
//     rsp->set_type(rpc::MsgType::RSP_RPC);
//     rsp->set_retcode(rpc::RetCode::SUCCESS);
//     rsp->set_result("result");
//     conn->send(rsp);
// }

int main() {
    auto dispatcer = std::make_shared<rpc::Dispatcher>();
    auto message_cb = std::bind(&rpc::Dispatcher::on_message, dispatcer.get(), std::placeholders::_1, std::placeholders::_2);
    dispatcer->register_handler<rpc::RpcRequest>(rpc::MsgType::REQ_RPC, on_rpc_request);
    dispatcer->register_handler<rpc::TopicRequest>(rpc::MsgType::REQ_TOPIC, on_topic_request);
    auto server = rpc::ServerFactory::create(9090);
    server->set_message_cb(message_cb);
    server->start();
    return 0;
}