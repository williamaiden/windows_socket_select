#pragma once

#include <windows.h>
#pragma comment(lib,"ws2_32.lib") 

class windows_socket_tcp_client
{
public:
	windows_socket_tcp_client(char* addr, int port);
	~windows_socket_tcp_client();
	int sendTcpData(const char* data, const int dataLength);
	int recvTcpData(char* data, int bufferLength);
private:
	SOCKET _tcp_socket;
};

