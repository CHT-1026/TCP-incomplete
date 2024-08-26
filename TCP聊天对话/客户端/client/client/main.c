#include<stdio.h>
#include<string.h>
#include<WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	WSADATA wsaDATA;
	WSAStartup(MAKEWORD(2, 2), &wsaDATA);

	SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == client_socket)
	{
		printf("create client socket failed!  error code:%d", GetLastError());
		return -1;
	}

	struct sockaddr_in target;
	target.sin_family = AF_INET;
	target.sin_port = htons(8080);
	target.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (-1 == connect(client_socket, (struct sockaddr*)&target, sizeof(target)))
	{
		printf("connect failed!  error code:%d", GetLastError());
		closesocket(client_socket);
		return -1;
	}


	while (1)
	{
		printf("enter the message");
		char s_buffer[1024] = { 0 };
		scanf("%s", s_buffer);
		send(client_socket, s_buffer, sizeof(s_buffer), 0);


		char r_buffer[1024] = { 0 };
		int ret = recv(client_socket, r_buffer, 1024, 0);

		if (ret <= 0) break;
		printf("%s\n", r_buffer);

	}
	closesocket(client_socket);






	return 0;
}