#include "windows_socket_tcp_client.h"
#include <stdio.h>

windows_socket_tcp_client::windows_socket_tcp_client(char* addr, int port){
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
		perror("WSASartup error !");
		return;
	}
	_tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_tcp_socket == INVALID_SOCKET){
		perror("socket error !");
		WSACleanup();
		return;
	}
	SOCKADDR_IN remote_config;
	remote_config.sin_port = htons(port);
	remote_config.sin_family = AF_INET;
	remote_config.sin_addr.S_un.S_addr = inet_addr(addr);
	if (connect(_tcp_socket, (sockaddr *)&remote_config, sizeof(remote_config)) == SOCKET_ERROR){
		perror("connect error !");
		closesocket(_tcp_socket);
		WSACleanup();
		return;
	}
}


windows_socket_tcp_client::~windows_socket_tcp_client(){
	if (_tcp_socket != INVALID_SOCKET){
		if (closesocket(_tcp_socket) == 0){
			_tcp_socket = INVALID_SOCKET;
			WSACleanup();
			return;
		}
		perror("close socket error.");
	}
}

int windows_socket_tcp_client::sendTcpData(const char* data, const int dataLength){
	if (data != NULL && _tcp_socket != INVALID_SOCKET && dataLength > 0){
		return send(_tcp_socket, data, dataLength, 0);
	}
	return 0;
}

int windows_socket_tcp_client::recvTcpData(char* data, int bufferLength){
	int ret = 0;
	if (data != NULL && _tcp_socket != INVALID_SOCKET && bufferLength > 0){
		ret = recv(_tcp_socket, data, bufferLength, 0);
		if (ret > 0){
			data[ret] = '\0';
			printf("%s\n", data);
		}
	}
	return ret;
}

int main(void){
	windows_socket_tcp_client c("127.0.0.1", 8086);

	char* strNum = "1234567890";
	c.sendTcpData(strNum, strlen(strNum));
	char recv[1024];
	c.recvTcpData(recv, sizeof(recv));

	char* strStr = "abcdefghi";
	c.sendTcpData(strStr, strlen(strStr));
	c.recvTcpData(recv, sizeof(recv));

	return 0;
}
