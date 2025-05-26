#include "../source/net/MessageFactory.hpp"
#include "../source/pbmessage/RpcMessage.pb.h"
#include <iostream>

using namespace std;

int main() {
    // rpc::RpcRequest::ptr req = std::dynamic_pointer_cast<rpc::RpcRequest>(rpc::MessageFactory::create(rpc::MsgType::REQ_RPC));
    rpc::ServiceRequest::ptr req = rpc::MessageFactory::create<rpc::ServiceRequest>();
    req->set_optype(rpc::ServiceOptype::DISCOVERY);
    req->set_method("方法1");
    req->set_address({"127.0.0.1", 8080});
    std::string str = req->serialize();
    cout << str << " " << str.size() << endl;

    //再反序列化
    rpc::ServiceRequest::ptr req1 = rpc::MessageFactory::create<rpc::ServiceRequest>();
    req1->deserialize(str);
    cout << static_cast<int>(req1->get_optype()) << endl;
    cout << req1->get_method() << endl;
    cout << req1->get_address().first << ":" << req1->get_address().second << endl;
    return 0;
}