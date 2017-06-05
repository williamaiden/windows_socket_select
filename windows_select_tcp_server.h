#pragma once

#include <WS2tcpip.h>
#include <WinSock2.H>  

#pragma comment(lib, "ws2_32.lib")   

class windows_select_tcp_server
{
public:
	windows_select_tcp_server();
	~windows_select_tcp_server();

	void windows_socket_select();
private:
	SOCKET _tcp_socket_listen;
};

