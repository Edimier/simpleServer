#include <string.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
 
#define PORT 9999
int main()
{
	int sockfd;
	struct sockaddr_in saddr;
	char recvline[1025] = {0};
	struct sockaddr_in presaddr;
	socklen_t len;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(PORT);
	bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
	while (1)
	{
		int ret = recvfrom(sockfd, recvline, sizeof(recvline), 0 , (struct sockaddr*)&presaddr, &len);
		if (ret <= 0)
		{
			printf("error\n");
			exit(-1);
		}
		printf("recvfrom %s,content=%s\n", inet_ntoa(presaddr.sin_addr), recvline);
	}
	return 0;
}