#include "../source/client/RpcClient.hpp"

void callback(const rpc::PBValue& result) {
    std::cout << result.number_value() << std::endl;
}

int main() {
    auto client = rpc::client::RpcClient::ptr(std::make_shared<rpc::client::RpcClient>(true, "127.0.0.1", 30002));


    std::vector<rpc::PBValue> params;
    rpc::PBValue param;
    param.set_number_value(11);
    params.push_back(param);
    param.set_number_value(22);
    params.push_back(param);
    rpc::PBValue result;

    bool ret = client->sync_call("Add", params, result);
    if (ret) {
        std::cout << result.number_value() << std::endl;
    }

    rpc::client::RpcCaller::PBAsyncResponse res_future;
    ret = client->async_call("Add", params, res_future);
    if (ret) {
        std::cout << res_future.get().number_value() << std::endl;
    }

    ret = client->callback_call("Add", params, callback);
    if (ret) {
        std::cout << "call success" << std::endl;
    }

    sleep(2);
    return 0;
}