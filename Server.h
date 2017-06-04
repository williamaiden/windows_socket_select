#pragma once

#include <WS2tcpip.h>
#include <WinSock2.H>  
#include <iostream>  

#pragma comment(lib, "ws2_32.lib")   

class Server
{
public:
	Server();
	~Server();

	void windows_socket_select();
private:
	SOCKET _socketListen;

};

