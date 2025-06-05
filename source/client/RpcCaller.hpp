#pragma once
#include "Requestor.hpp"
#include "../util/uuid.hpp"

namespace rpc {
    namespace client {
        class RpcCaller {
        public:
            using ptr = std::shared_ptr<RpcCaller>;
            using PBAsyncResponse = std::future<PBValue>;
            using PBResponseCallback = std::function<void(const PBValue&)>;

            RpcCaller(const Requestor::ptr& _requestor)
            : requestor(_requestor) {}

            // 同步调用
            bool call(const BaseConnection::ptr& _conn, const std::string& _method, const std::vector<PBValue>& _params, PBValue& _result) {
                // 组织请求数据
                RpcRequest::ptr req = MessageFactory::create<RpcRequest>();
                req->set_id(UUID::ramdom());
                req->set_type(MsgType::REQ_RPC);
                req->set_method(_method);
                req->set_params(_params);
                // 发送请求
                BaseMessage::ptr base_rsp;
                bool ret = requestor->sync_send(_conn, req, base_rsp);
                if(!ret) {
                    logging.error("RpcCaller::call 发送请求失败");
                    return false;
                }
                // 等待响应
                RpcResponse::ptr rsp = std::dynamic_pointer_cast<RpcResponse>(base_rsp);
                if(!rsp) {
                    logging.error("RpcCaller::call 响应类型错误");
                    return false;
                }
                _result = rsp->get_result();

                if(rsp->get_retcode() != RetCode::SUCCESS) {
                    logging.error("RpcCaller::call 请求失败, 错误码:{}", err_reason(rsp->get_retcode()));
                    return false;
                }
                return true;
            }

            //异步调用
            bool call(const BaseConnection::ptr& _conn, const std::string& _method, const std::vector<PBValue>& _params, PBAsyncResponse& _result) {
                RpcRequest::ptr req = MessageFactory::create<RpcRequest>();
                req->set_id(UUID::ramdom());
                req->set_type(MsgType::REQ_RPC);
                req->set_method(_method);
                req->set_params(_params);
                // 发送请求
                auto pb_promise = std::make_shared<std::promise<PBValue>>();
                _result = pb_promise->get_future();
                Requestor::RequestCallBack callback = std::bind(&RpcCaller::async_callback, this, std::placeholders::_1, pb_promise);
                bool ret = requestor->callback_send(_conn, req, callback);
                if(!ret) {
                    logging.error("RpcCaller::call 发送请求失败");
                    return false;
                }
                return true;
            } 

            // 回调
            bool call(const BaseConnection::ptr& _conn, const std::string& _method, const std::vector<PBValue>& _params, const PBResponseCallback& _cb) {
                // 组织请求数据
                RpcRequest::ptr req = MessageFactory::create<RpcRequest>();
                req->set_id(UUID::ramdom());
                req->set_type(MsgType::REQ_RPC);
                req->set_method(_method);
                req->set_params(_params);
                Requestor::RequestCallBack callback = std::bind(&RpcCaller::callback, this, std::placeholders::_1, _cb);
                bool ret = requestor->callback_send(_conn, req, callback);
                if(!ret) {
                    logging.error("RpcCaller::call 发送请求失败");
                    return false;
                }
                return true;
            }
        private:
            void async_callback(const BaseMessage::ptr& _msg, std::shared_ptr<std::promise<PBValue>>& _result) {
                RpcResponse::ptr rsp = std::dynamic_pointer_cast<RpcResponse>(_msg);
                if(!rsp) {
                    logging.error("RpcCaller::callback 响应类型错误");
                    return;
                }
                if(rsp->get_retcode() != RetCode::SUCCESS) {
                    logging.error("RpcCaller::callback 请求失败, 错误码:{}", err_reason(rsp->get_retcode()));
                    return;
                }
                _result->set_value(rsp->get_result());
            }

            void callback(const BaseMessage::ptr& _msg, const PBResponseCallback& _cb) {
                RpcResponse::ptr rsp = std::dynamic_pointer_cast<RpcResponse>(_msg);
                if(!rsp) {
                    logging.error("RpcCaller::callback 响应类型错误");
                    return;
                }
                if(rsp->get_retcode() != RetCode::SUCCESS) {
                    logging.error("RpcCaller::callback 请求失败, 错误码:{}", err_reason(rsp->get_retcode()));
                    return;
                }
                _cb(rsp->get_result());
            }
        private:
            Requestor::ptr requestor;
        };
    }
}