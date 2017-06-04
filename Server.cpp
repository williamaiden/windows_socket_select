#include "Server.h"


Server::Server()
{
	/// 初始化socket  
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);
	int result = 0;
	result = WSAStartup(version, &wsaData);
	if (result != 0){
		std::cout << "WSAStartup() error." << std::endl;
		return;
	}

	/// 创建socket   
	//SOCKET socketListen;
	_socketListen = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketListen == INVALID_SOCKET){
		WSACleanup();
		std::cout << "socket() error." << std::endl;
		return;
	}

	/// 服务器地址结构   
	sockaddr_in svrAddress;
	svrAddress.sin_family = AF_INET;
	svrAddress.sin_addr.s_addr = INADDR_ANY;
	svrAddress.sin_port = htons(8086);

	/// 绑定服务器套接字   
	result = bind(_socketListen, (sockaddr*)&svrAddress, sizeof(svrAddress));
	if (result == SOCKET_ERROR){
		closesocket(_socketListen);
		WSACleanup();
		std::cout << "bind() error." << std::endl;
		return;
	}

	/// 开启监听  
	result = listen(_socketListen, 5);
	if (result == SOCKET_ERROR){
		closesocket(_socketListen);
		WSACleanup();
		std::cout << "listen() error." << std::endl;
		return;
	}
	std::cout << "服务器启动成功，监听端口：" << ntohs(svrAddress.sin_port) << std::endl;
}

Server::~Server(){
}
///Windows socket select使用步骤
///1、ED_ZERO，初始化readfds/witerfds/exceptfds/fd_set
///2、FD_SET，将socket加入set
///3、TimeOut设置，即timeval
///4、select，调用
///5、select return，0表示超时、SOCKET_ERROR表示失败；
///6、ED_ISSET，检查(见下文)
///7、socket status -->goto (1)
void Server::windows_socket_select(){
	/// select模型   
	fd_set allSockSet;
	FD_ZERO(&allSockSet);///1
	FD_SET(_socketListen, &allSockSet); ///2

	while (true){
		fd_set readSet;///1
		FD_ZERO(&readSet);///2
		readSet = allSockSet;
		/*
		批注1：@William Aiden 2017-6-3
			每次调用select，无论成功与否，都要重新初始化fd_set，并将socket放入fd_set中
			因为select通过修改fd_set来报告结果.
		*/
		
		int result = select(0, &readSet, NULL, NULL, NULL);
		/*
		批注2：
			select(0,readfds,writefds,exceptfds,timeval)
			1、select第一个参数WinSockt并没有使用，但是为了与Berkeley Sockets兼容，
			形式上保留它，可以传入任意int值.
			2、timeval = NULL，表示无限阻塞
				timeval tm;
				tm.tv_sec = 1L;   //s
				tm.tv_usec = 100L;//ms
		*/
		if (result == SOCKET_ERROR){///SOCKET_ERROR == -1
			std::cout << "listen() error." << std::endl;
			break;
		}
		/*
		批注3：
			select有三种返回值，(1)SOCKET_ERROR、(2)0 - 超时、(3)有效
		*/
		if (result == 0){
			std::cout << "listen() time out." << std::endl;
			break;
		}
		/*
		批注4：
			FD_ISSET():检查某个socket句柄是否是一个fd_set成员.
		*/
		if (FD_ISSET(_socketListen, &readSet)){
			sockaddr_in clientAddr;
			int len = sizeof(clientAddr);
			///一个客户端连接请求
			SOCKET clientSocket = accept(_socketListen, (sockaddr*)&clientAddr, &len);
			if (clientSocket == INVALID_SOCKET){
				std::cout << "accept() error." << std::endl;
				break;
			}
			FD_SET(clientSocket, &allSockSet);   /// 将新创建的套接字加入到集合中   

			char ipAddress[16] = { 0 };
			inet_ntop(AF_INET, &clientAddr, ipAddress, 16);
			std::cout << "有新的连接[" << ipAddress << ":" << ntohs(clientAddr.sin_port)
				<< "], 目前客户端的数量为：" << allSockSet.fd_count - 1 << std::endl;

			continue;
		}

		for (u_int i = 0; i < allSockSet.fd_count; ++i)
		{
			SOCKET socket = allSockSet.fd_array[i];

			sockaddr_in clientAddr;
			int len = sizeof(clientAddr);
			getpeername(socket, (struct sockaddr *)&clientAddr, &len);
			char ipAddress[16] = { 0 };
			inet_ntop(AF_INET, &clientAddr, ipAddress, 16);

			/// 可读性监视，可读性指有连接到来、有数据到来、连接已关闭、重置或终止  
			if (FD_ISSET(socket, &readSet))
			{
				char bufRecv[100];
				result = recv(socket, bufRecv, 100, 0);
				if (result == SOCKET_ERROR)
				{
					DWORD err = WSAGetLastError();
					if (err == WSAECONNRESET)       /// 客户端的socket没有被正常关闭,即没有调用closesocket  
					{
						std::cout << "客户端[" << ipAddress << ":" << ntohs(clientAddr.sin_port) << "]被强行关闭, ";
					}
					else
					{
						std::cout << "recv() error，" << std::endl;
					}

					closesocket(socket);
					FD_CLR(socket, &allSockSet);

					std::cout << "目前客户端的数量为：" << allSockSet.fd_count - 1 << std::endl;
					break;
				}
				else if (result == 0)               /// 客户端的socket调用closesocket正常关闭  
				{
					closesocket(socket);
					FD_CLR(socket, &allSockSet);

					std::cout << "客户端[" << ipAddress << ":" << ntohs(clientAddr.sin_port)
						<< "]已经退出，目前客户端的数量为：" << allSockSet.fd_count - 1 << std::endl;
					break;
				}

				bufRecv[result] = '\0';
				std::cout << "来自客户端[" << ipAddress << ":" << ntohs(clientAddr.sin_port)
					<< "]的消息：" << bufRecv << std::endl;
			}
		}
	}

	for (u_int i = 0; i < allSockSet.fd_count; ++i)
	{
		SOCKET socket = allSockSet.fd_array[i];
		closesocket(socket);
	}

	WSACleanup();
}

int main(){
	Server s;
	s.windows_socket_select();

	return 0;
}
