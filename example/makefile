# all : test_net_client test_net_server

# test_net_client:
# 	g++ -std=c++17 -g -o test_net_client test_net_client.cpp /home/emilia/Desktop/Code/rpc/source/net/pbmessage/RpcMessage.pb.cc -lpthread -lprotobuf

# test_net_server:
# 	g++ -std=c++17 -o test_net_server test_net_server.cpp /home/emilia/Desktop/Code/rpc/source/net/pbmessage/RpcMessage.pb.cc -lpthread -lprotobuf

all : test_rpc_client test_rpc_server

test_rpc_client:
	g++ -std=c++17 -g -o test_rpc_client test_rpc_client.cpp /home/emilia/Desktop/Code/rpc/source/net/pbmessage/RpcMessage.pb.cc -lpthread -lprotobuf

test_rpc_server:
	g++ -std=c++17 -g -o test_rpc_server test_rpc_server.cpp /home/emilia/Desktop/Code/rpc/source/net/pbmessage/RpcMessage.pb.cc -lpthread -lprotobuf
