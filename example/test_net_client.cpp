#include "../source/net/factory/ClientFactory.hpp"
#include "../source/net/factory/ConnectionFactory.hpp"
#include "../source/net/factory/MessageFactory.hpp"
#include "../source/net/factory/BufferFactory.hpp"
#include "../source/net/factory/ProtocolFactory.hpp"
#include "../source/dispatcher/Dispatcher.hpp"

void on_rpc_response(const rpc::BaseConnection::ptr& conn, const rpc::RpcResponse::ptr& msg) {
    std::string body = msg->serialize();
    std::cout << "收到了RPC响应" << std::endl;
    std::cout << body << std::endl;
}

void on_topic_response(const rpc::BaseConnection::ptr& conn, const rpc::TopicResponse::ptr& msg) {
    std::string body = msg->serialize();
    std::cout << "收到了Topic响应" << std::endl;
    std::cout << body << std::endl;
}

// void on_message(const rpc::BaseConnection::ptr& conn, const rpc::BaseMessage::ptr& msg) {
//     std::string body = msg->serialize();
//     std::cout << body << std::endl;
// }

int main() {
    auto dispatcer = std::make_shared<rpc::Dispatcher>();
    dispatcer->register_handler<rpc::RpcResponse>(rpc::MsgType::RSP_RPC, on_rpc_response);
    dispatcer->register_handler<rpc::TopicResponse>(rpc::MsgType::RSP_TOPIC, on_topic_response);
    auto client = rpc::ClientFactory::create("127.0.0.1", 9090);
    auto message_cb = std::bind(&rpc::Dispatcher::on_message, dispatcer.get(), std::placeholders::_1, std::placeholders::_2);
     client->set_message_cb(message_cb);
    client->connect();

    auto req = rpc::MessageFactory::create<rpc::RpcRequest>();
    req->set_id("id");
    req->set_method("method");
    req->set_params({"param1", "param2"});
    for(int i = 0; i < 1e3; i++){
        if(i % 2 == 1){
            req->set_type(rpc::MsgType::REQ_RPC);
        }
        else {
            req->set_type(rpc::MsgType::REQ_TOPIC);
        }
        client->send(req);
    }
    sleep(5);
    client->shutdown();
    sleep(5);
    return 0;
}