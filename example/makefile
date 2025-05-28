all : test_net_client test_net_server

test_net_client:
	g++ -std=c++17 -g -o test_net_client test_net_client.cpp /home/emilia/Code/rpc/source/net/pbmessage/RpcMessage.pb.cc -lpthread -lprotobuf

test_net_server:
	g++ -std=c++17 -o test_net_server test_net_server.cpp /home/emilia/Code/rpc/source/net/pbmessage/RpcMessage.pb.cc -lpthread -lprotobuf