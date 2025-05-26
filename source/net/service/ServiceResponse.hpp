#pragma once
#include "concrete/ProtoResponse.hpp"

namespace rpc
{
    class ServiceResponse : public ProtoResponse
    {
    public:
        using ptr = std::shared_ptr<ServiceResponse>;
        using Address = std::pair<std::string, uint16_t>;

        ServiceResponse() : ProtoResponse(new msg::ServiceResponse()) {}

        virtual bool check() override {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* retcode_field = descriptor->FindFieldByName("retcode");
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName("optype");
            // 返回码字段存在，并且返回码类型为int32
            if (!retcode_field || retcode_field->cpp_type() != PBFieldDescriptor::CPPTYPE_INT32) {
                logging.error("RpcResponse 返回码为空或类型错误!");
                return false;
            }
            // 操作类型字段存在，并且操作类型类型为int32
            if (!optype_field || optype_field->cpp_type() != PBFieldDescriptor::CPPTYPE_INT32) {
                logging.error("RpcResponse 操作类型为空或类型错误!");
                return false;
            }
            // 操作类型为DISCOVERY，方法名和地址列表必须存在
            const PBReflection* reflection = message->GetReflection();
            if (reflection->GetInt32(*message, optype_field) == static_cast<int>(ServiceOptype::DISCOVERY)) {
                // 服务名字段不存在
                if (!reflection->HasField(*message, descriptor->FindFieldByName("method"))) {
                    logging.error("RpcResponse 发现服务请求, 方法名为空!");
                    return false;
                }
                // 地址字段不存在或者地址字段不为repeated string
                if(!reflection->HasField(*message, descriptor->FindFieldByName("address")))
                {
                    logging.error("RpcResponse 发现服务请求, 地址列表为空!");
                    return false;
                }
                const PBFieldDescriptor* address_field = descriptor->FindFieldByName("address");
                if (address_field->cpp_type() != PBFieldDescriptor::CPPTYPE_MESSAGE) {
                    logging.error("RpcResponse 发现服务请求, 地址列表类型错误!");
                    return false;
                }
            }
            return true;
        }

        std::string get_method() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName("method");
            return message->GetReflection()->GetString(*message, method_field);
        }

        void set_method(const std::string& _method) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName("method");
            message->GetReflection()->SetString(message.get(), method_field, _method);
        }

        ServiceOptype get_optype() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName("optype");
            return static_cast<ServiceOptype>(message->GetReflection()->GetInt32(*message, optype_field));
        }

        void set_optype(ServiceOptype _optype) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName("optype");
            message->GetReflection()->SetInt32(message.get(), optype_field, static_cast<int>(_optype));
        }

        std::vector<Address> get_address() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* address_field = descriptor->FindFieldByName("address");
            const PBReflection* reflection = message->GetReflection();
            std::vector<Address> address_list;
            
            for (int i = 0; i < reflection->FieldSize(*message, address_field); ++i) {
                const PBMessage& addr = reflection->GetRepeatedMessage(*message, address_field, i);
                std::string ip = addr.GetReflection()->GetString(addr, addr.GetDescriptor()->FindFieldByName("ip"));
                int port = static_cast<int>(addr.GetReflection()->GetInt32(addr, addr.GetDescriptor()->FindFieldByName("port")));
                address_list.push_back({ip, port});
            }
            return address_list;
        }

        void set_address(const std::vector<Address>& _address) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* address_field = descriptor->FindFieldByName("address");
            const PBReflection* reflection = message->GetReflection();
            // 清空现有的 repeated 字段
            reflection->ClearField(message.get(), address_field);
            // 添加新的地址到 repeated 字段
            for (const auto& addr : _address) {
                PBMessage* address_msg = reflection->AddMessage(message.get(), address_field);
                address_msg->GetReflection()->SetString(address_msg, address_msg->GetDescriptor()->FindFieldByName("ip"), addr.first);
                address_msg->GetReflection()->SetInt32(address_msg, address_msg->GetDescriptor()->FindFieldByName("port"), addr.second);
            }
        }

    };
}