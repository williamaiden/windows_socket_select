#include "windows_select_tcp_server.h"
#include <stdio.h>

windows_select_tcp_server::windows_select_tcp_server(){
	/// 初始化socket  
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);
	if (WSAStartup(version, &wsaData) != 0){
		perror("WSAStartup() error.");
		return;
	}
	/// 创建socket   
	//SOCKET _tpc_socket_listen;
	_tcp_socket_listen = socket(AF_INET, SOCK_STREAM, 0);
	/*
	批注1：
	当socket第三个参数为0时，会自动选择第二个参数类型对应的默认协议.
	*/
	if (_tcp_socket_listen == INVALID_SOCKET){
		WSACleanup();
		perror("socket() error.");
		return;
	}

	/// 服务器地址结构   
	sockaddr_in local_addr_config;
	local_addr_config.sin_family = AF_INET;
	local_addr_config.sin_addr.s_addr = INADDR_ANY;
	local_addr_config.sin_port = htons(8086);

	/// 绑定服务器套接字   
	if (bind(_tcp_socket_listen, (sockaddr*)&local_addr_config, sizeof(local_addr_config)) == SOCKET_ERROR){
		closesocket(_tcp_socket_listen);
		WSACleanup();
		perror("bind() error.");
		return;
	}
	/// 开启监听  
	if (listen(_tcp_socket_listen, SOMAXCONN) == SOCKET_ERROR){
		closesocket(_tcp_socket_listen);
		WSACleanup();
		perror("listen() error.");
		return;
	}
	printf("正在监听...端口：%u\n", ntohs(local_addr_config.sin_port));
}

windows_select_tcp_server::~windows_select_tcp_server(){
	if (_tcp_socket_listen != INVALID_SOCKET){
		if (closesocket(_tcp_socket_listen) == 0){
			_tcp_socket_listen = INVALID_SOCKET;
			WSACleanup();
			return;
		}
		perror("close socket error.");
	}
}

///Windows socket select使用步骤
///1、ED_ZERO，初始化readfds/witerfds/exceptfds/fd_set
///2、FD_SET，将socket加入set
///3、TimeOut设置，即timeval
///4、select，调用
///5、select return，0表示超时、SOCKET_ERROR表示失败；
///6、FD_ISSET，检查(见下文)
///7、socket status -->goto (1)
void windows_select_tcp_server::windows_socket_select(){
	/// select模型   
	fd_set allSockSet;  
	FD_ZERO(&allSockSet);
	/*
	批注2：
		FD_ZERO():清除fd_set中所有内容.
	*/
	FD_SET(_tcp_socket_listen, &allSockSet);
	/*
	批注3：
		FD_SET():把一个socket句柄加入fd_set中.
		这里将_tcp_socket_listen加入fd_set中，避免select一个全空的socket，造成CPU暴涨.
		fd_set并非Map，实际上是一个long数组，顾有个add先后关系，[0]即为_tcp_socket_listen本身
	*/
	bool flag = true;
	while (flag){
		fd_set readSet;
		FD_ZERO(&readSet);
		readSet = allSockSet;
		/*
		批注4：@William Aiden 2017-6-3
		每次调用select，无论成功与否，都要重新初始化fd_set，并将socket放入fd_set中
		因为select通过修改fd_set来报告结果.
		*/

		int select_ret = select(0, &readSet, NULL, NULL, NULL);
		/*
		批注5：
		select(0,readfds,writefds,exceptfds,timeval)
		1、select第一个参数WinSockt并没有使用，但是为了与Berkeley Sockets兼容，
		形式上保留它，可以传入任意int值.
		2、timeval = NULL，表示无限阻塞
		timeval tm;
		tm.tv_sec = 1L;   //s
		tm.tv_usec = 100L;//ms
		*/

		if (select_ret == SOCKET_ERROR){///SOCKET_ERROR == -1
			perror("listen() error.");
			break;//jump out while
		}
		/*
		批注6：
		select有三种返回值，(1)SOCKET_ERROR、(2)0 - 超时、(3)有效
		这里不用switch，因为break无法跳出while循环，只能跳出switch本身
		*/
		if (select_ret == 0){
			perror("listen() time out.");
			break;//jump out while
		}
		/*
		批注7：
		FD_ISSET():检查某个socket句柄是否是一个fd_set成员.
		*/
		if (FD_ISSET(_tcp_socket_listen, &readSet)){
			///_tcp_socket_listen 是用来监听客户端accept连接请求的
			sockaddr_in client_addr_config;
			int addr_len = sizeof(client_addr_config);
			///一个客户端连接请求
			SOCKET client_tcp_socket = accept(_tcp_socket_listen, (sockaddr*)&client_addr_config, &addr_len);
			if (client_tcp_socket == INVALID_SOCKET){
				perror("accept() error.");
				break;
			}
			/// 将新创建的套接字加入到集合中
			FD_SET(client_tcp_socket, &allSockSet);   
			///提示
			char ip_addr[16] = { 0 };
			inet_ntop(AF_INET, &client_addr_config, ip_addr, sizeof(ip_addr));
			printf("有新的连接[%s:%u],目前客户端的数量为：%u\n", ip_addr, 
				ntohs(client_addr_config.sin_port), allSockSet.fd_count - 1);
		}
		/*
		批注8：
			fd_set实际为long数组
			轮询，int i = 0，表示_tcp_socket_listen本身，顾从i = 1开始为连接上的客户端的socket
		*/
		for (u_int i = 1; i < allSockSet.fd_count; ++i) {
			SOCKET client_tcp_socket = allSockSet.fd_array[i];
			sockaddr_in client_addr_config;
			int addr_len = sizeof(client_addr_config);
			///getpeername获取client地址
			getpeername(client_tcp_socket, (struct sockaddr *)&client_addr_config, &addr_len);
			char ip_addr[16] = { 0 };
			///inet_ntop将ip地址从2进制->点10进制
			inet_ntop(AF_INET, &client_addr_config, ip_addr, sizeof(ip_addr));
			/// 可读性监视，可读性指有连接到来、有数据到来、连接已关闭、重置或终止  
			if (FD_ISSET(client_tcp_socket, &readSet)){
				char recvBuffer[1024];
				int recv_ret = recv(client_tcp_socket, recvBuffer, sizeof(recvBuffer), 0);
				if (recv_ret == SOCKET_ERROR){
					DWORD err = WSAGetLastError();
					if (err == WSAECONNRESET){
						/// 客户端的socket没有被正常关闭,即没有调用closesocket
						printf("客户端[%s:%u]被强行关闭.\n", ip_addr, ntohs(client_addr_config.sin_port));
					}
					else{
						perror("recv() error.");
					}
					closesocket(client_tcp_socket);
					/*
					批注9：
						FD_CLR():从一个fd_set中删除一个socket句柄.
					*/
					FD_CLR(client_tcp_socket, &allSockSet);
					printf("目前客户端的数量为：%u\n", allSockSet.fd_count - 1);
					break;//jump out for
				}
				else if (recv_ret == 0) {
					/// 客户端的socket调用closesocket正常关闭  
					closesocket(client_tcp_socket);
					FD_CLR(client_tcp_socket, &allSockSet);
					printf("客户端[%s:%u]已经退出，目前客户端的数量为：%u.\n", ip_addr, 
						ntohs(client_addr_config.sin_port), allSockSet.fd_count - 1);
					break;//jump out for
				}
				recvBuffer[recv_ret] = '\0';
				printf("来自客户端[%s:%u]的消息：%s\n", ip_addr,
					ntohs(client_addr_config.sin_port), recvBuffer);
				if (send(client_tcp_socket, recvBuffer, strlen(recvBuffer), 0) <= 0){
					perror("send() error.");
				}
			}
		}
	}
	/*
	批注10：
		accept出现故障，关闭所有Socket
	*/
	for (u_int i = 1; i < allSockSet.fd_count; ++i) {
		SOCKET socket_set = allSockSet.fd_array[i];
		if (socket_set != INVALID_SOCKET){
			closesocket(socket_set);
		}
		FD_CLR(socket_set, &allSockSet);
	}
	///卸载Socket套接字库
	WSACleanup();
}

int main(){
	windows_select_tcp_server s;
	s.windows_socket_select();

	return 0;
}
