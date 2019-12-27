#include <string.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
 
#define SERVER_IP "192.168.0.255"
#define SERVER_PORT 9999

int main()
{
	int sockfd;
	struct sockaddr_in des_addr;
	int r;
	char sendline[1024] = {"Hello"};
	int opt = 1;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)); //设置套接字选项
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr.s_addr = inet_addr(SERVER_IP); //广播地址
	des_addr.sin_port = htons(SERVER_PORT);
	int ret = sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr*)&des_addr, sizeof(des_addr));
	if (ret <= 0)
	{
		printf("error\n");
		exit(-1);
	}
	printf("finish\n");
	return 0;
}