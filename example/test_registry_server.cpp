#include "../source/common/Fields.hpp"
#include "../source/server/RpcServer.hpp"
#include "../source/server/RpcRegistry.hpp"

int main() {
    rpc::server::RegistryServer server(rpc::Address("127.0.0.1", 30002));
    server.start();
    return 0;
}