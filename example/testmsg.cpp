#include "../source/service/MessageFactory.hpp"

int main() {
    auto rreq = rpc::MessageFactory::create<msg::RpcRequest>();
}