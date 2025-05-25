#pragma once
#include "../concrete/ProtoRequest.hpp"
#include "../pbmessage/RpcRequest.pb.h"

namespace rpc
{
    class ServiceRequest : public ProtoRequest
    {
    public:
        using ptr = std::shared_ptr<ServiceRequest>;
        using Address = std::pair<std::string, int>;

        // 服务请求: 服务名称和操作类型
        virtual bool check() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName(key::method);
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName(key::optype);
            const PBFieldDescriptor* address_field = descriptor->FindFieldByName(key::address);
            if (!method_field || method_field->cpp_type() != PBFieldDescriptor::CPPTYPE_STRING) {
                logging.error("ServiceRequest 服务名称为空!");
                return false;
            }
            if (!optype_field || optype_field->cpp_type() != PBFieldDescriptor::CPPTYPE_INT32) {
                logging.error("ServiceRequest 操作类型为空!");
                return false;
            }
            if (!address_field || address_field->cpp_type() != PBFieldDescriptor::CPPTYPE_MESSAGE) {
                logging.error("ServiceRequest 地址为空!");
                return false;
            }
            return true;
        }

        std::string get_method() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName(key::method);
            const PBReflection* reflection = message->GetReflection();
            return reflection->GetString(*message, method_field);
        }

        void set_method(const std::string& _method) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* method_field = descriptor->FindFieldByName(key::method);
            const PBReflection* reflection = message->GetReflection();
            reflection->SetString(message, method_field, _method);
        }

        ServiceOptype get_optype() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName(key::optype);
            const PBReflection* reflection = message->GetReflection();
            return static_cast<ServiceOptype>(reflection->GetInt32(*message, optype_field));
        }

        void set_optype(ServiceOptype _optype) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* optype_field = descriptor->FindFieldByName(key::optype);
            const PBReflection* reflection = message->GetReflection();
            reflection->SetInt32(message, optype_field, static_cast<int>(_optype));
        }
        
// ServiceRequest:
    // message ServiceRequest {
    //     //..........
    //     optional Address address = 3;
    //     message Address {
    //         optional string ip = 1;
    //         optional uint32 port = 2;
    //     }
    // }

        Address get_address() {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* address_field = descriptor->FindFieldByName(key::address);
            const PBReflection* reflection = message->GetReflection();
            const PBMessage& address = reflection->GetMessage(*message, address_field);
            const PBDescriptor* address_descriptor = address.GetDescriptor();
            const PBFieldDescriptor* ip_field = address_descriptor->FindFieldByName(key::ip);
            const PBFieldDescriptor* port_field = address_descriptor->FindFieldByName(key::port);
            const PBReflection* address_reflection = address.GetReflection();
            return std::make_pair(address_reflection->GetString(address, ip_field), address_reflection->GetUInt32(address, port_field));
        }

        void set_address(const Address& _address) {
            const PBDescriptor* descriptor = message->GetDescriptor();
            const PBFieldDescriptor* address_field = descriptor->FindFieldByName(key::address);
            const PBReflection* reflection = message->GetReflection();
            PBMessage* address = reflection->MutableMessage(message, address_field);
            const PBDescriptor* address_descriptor = address->GetDescriptor();
            const PBFieldDescriptor* ip_field = address_descriptor->FindFieldByName(key::ip);
            const PBFieldDescriptor* port_field = address_descriptor->FindFieldByName(key::port);
            const PBReflection* address_reflection = address->GetReflection();
            address_reflection->SetString(address, ip_field, _address.first);
            address_reflection->SetUInt32(address, port_field, _address.second);
        }
    };
}