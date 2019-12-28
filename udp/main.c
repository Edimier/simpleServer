#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
#define BUFF_LEN 1024

typedef struct UDPCLIENT
{
	struct sockaddr_in m_clent_addr;
	int 			   m_fd;
	char 			   m_data[BUFF_LEN];
	int 			   m_len;
}UDPClint;

void * workproc(void *ptr)
{
	UDPClint * udpclient = (UDPClint *)ptr;
	printf("recieve client msg=%s\n",udpclient->m_data);
	int len = sizeof(udpclient->m_clent_addr);
    sendto(udpclient->m_fd, udpclient->m_data, udpclient->m_len, 0, (struct sockaddr*)&(udpclient->m_clent_addr), len);
}

void handle_udp_msg(int fd)
{
    char buf[BUFF_LEN];
    socklen_t len;
    int count;
    struct sockaddr_in clent_addr;
    while(1)
    {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(clent_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, &len);
        if(count == -1)
        {
            printf("recieve data fail!\n");
            return;
        }

        UDPClint udpclient;
        memset(&udpclient, 0, sizeof(UDPClint));
        memcpy(&udpclient.m_clent_addr, &clent_addr, sizeof(struct sockaddr_in));
        udpclient.m_fd = fd;
        memcpy(udpclient.m_data, buf, count);
        udpclient.m_len = count;

        pthread_t tid;
   		pthread_attr_t attr;
    	pthread_attr_init(&attr);
    	// 分离状态
   	 	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    	pthread_create(&tid, &attr, workproc, (void*)&udpclient);

        // printf("recieve client msg=%s\n",buf);
        // sendto(fd, buf, count, 0, (struct sockaddr*)&clent_addr, len);
    }
}

int main(int argc, char* argv[])
{
    int server_fd, ret;
    struct sockaddr_in ser_addr; 

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    printf("server port=%d\n", SERVER_PORT);

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(SERVER_PORT);

    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }

    handle_udp_msg(server_fd);

    close(server_fd);
    return 0;
}