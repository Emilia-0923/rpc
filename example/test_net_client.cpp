#include "../source/net/factory/ClientFactory.hpp"
#include "../source/net/factory/ConnectionFactory.hpp"
#include "../source/net/factory/MessageFactory.hpp"
#include "../source/net/factory/BufferFactory.hpp"
#include "../source/net/factory/ProtocolFactory.hpp"

void on_message(const rpc::BaseConnection::ptr& conn, const rpc::BaseMessage::ptr& msg) {
    std::string body = msg->serialize();
    std::cout << body << std::endl;
}

int main() {
    auto client = rpc::ClientFactory::create("127.0.0.1", 9090);
    client->set_message_cb(on_message);
    client->connect();

    auto req = rpc::MessageFactory::create<rpc::RpcRequest>();
    req->set_id("id");
    req->set_method("method");
    req->set_type(rpc::MsgType::REQ_RPC);
    req->set_params({"param1", "param2"});
    for(int i = 0; i < 5; i++){
        client->send(req);
    }
    sleep(1);
    client->shutdown();
    return 0;
}