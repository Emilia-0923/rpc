#pragma once
#include "../net/abstract/BaseConnection.hpp"
#include "../net/factory/MessageFactory.hpp"
#include <future>

namespace rpc {
    namespace client {
        class Requestor {
        public:
            using ptr = std::shared_ptr<Requestor>;

            using RequestCallBack = std::function<void(const BaseMessage::ptr&)>;
            using AsyncResponse = std::future<BaseMessage::ptr>;
            struct RequestDescribe {
                using ptr = std::shared_ptr<RequestDescribe>;
                
                BaseMessage::ptr request;
                RpcType rpc_type;
                std::promise<BaseMessage::ptr> response;
                RequestCallBack callback;
            };
            
            void on_response(const BaseConnection::ptr& _conn, const BaseMessage::ptr& _req) {
                auto desc = get_describe(_req->get_id());
                if (!desc) {
                    logging.error("Requestor::on_response 没有找到请求描述");
                }
                else {
                    if(desc->rpc_type == RpcType::ASYNC) {
                        desc->response.set_value(_req);
                    }
                    else if (desc->rpc_type == RpcType::CALLBACK) {
                        if (desc->callback) {
                            desc->callback(_req);
                        }
                        else {
                            logging.warning("Requestor::on_response 没有回调函数");
                        }
                    }
                    else {
                        logging.error("Requestor::on_response 未知请求类型");
                    }
                }
                remove_describe(_req->get_id());
            }

            // 同步发送请求
            bool sync_send(const BaseConnection::ptr& _conn, const BaseMessage::ptr& _req, BaseMessage::ptr& _rsp) {
                AsyncResponse async_rsp;
                bool ret = async_send(_conn, _req, async_rsp);
                if (!ret) {
                    logging.error("Requestor::send 发送请求失败");
                    return false;
                }
                _rsp = async_rsp.get();
                return true;
            }

            // 异步发送请求
            bool async_send(const BaseConnection::ptr& _conn, const BaseMessage::ptr& _req, AsyncResponse& _async_rsp) {
                RequestDescribe::ptr desc = new_describe(_req, RpcType::ASYNC);
                if (!desc) {
                    logging.error("Requestor::send 创建请求描述失败");
                    return false;
                }
                _conn->send(_req);
                _async_rsp = desc->response.get_future();
                return true;
            }

            bool callback_send(const BaseConnection::ptr& _conn, const BaseMessage::ptr& _req, const RequestCallBack& _cb) {
                RequestDescribe::ptr desc = new_describe(_req, RpcType::CALLBACK, _cb);
                if (!desc) {
                    logging.error("Requestor::send 创建请求描述失败");
                    return false;
                }
                _conn->send(_req);
                return true;
            }
        private:
            RequestDescribe::ptr new_describe(const BaseMessage::ptr& _req, RpcType _type, const RequestCallBack& _cb = nullptr) {
                std::lock_guard<std::mutex> lock(mtx);
                RequestDescribe::ptr desc = std::make_shared<RequestDescribe>();
                desc->request = _req;
                desc->rpc_type = _type;
                if( _type == RpcType::CALLBACK) {
                    desc->callback = _cb;
                }
                requset_desc[_req->get_id()] = desc;
                return desc;
            }

            RequestDescribe::ptr get_describe(const std::string& _req_id) {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = requset_desc.find(_req_id);
                if (it != requset_desc.end()) {
                    return it->second;
                }
                return nullptr;
            }

            void remove_describe(const std::string& _req_id) {
                std::lock_guard<std::mutex> lock(mtx);
                requset_desc.erase(_req_id);
            }
        private:
            std::mutex mtx;
            std::unordered_map<std::string, RequestDescribe::ptr> requset_desc;
        };
    }
}