#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#define SERVER_PORT 8888
#define BUFF_LEN 512
#define SERVER_IP "127.0.0.1"

#ifdef CONNECT
void udp_msg_sender(int fd, struct sockaddr* dst)
{
    pid_t pid = getpid();
    connect(fd, dst, sizeof(*dst));
    socklen_t len;
    struct sockaddr_in src;

    char buf[BUFF_LEN] = {0};

    sprintf(buf, "hello,i am %d", pid);

    while(1)
    {
        // memset(buf, 0, BUFF_LEN);
        // printf("input client msg=");
        // scanf("%s", buf);
        len = sizeof(*dst);
        write(fd, buf, strlen(buf));

        memset(buf, 0, BUFF_LEN);
        int ret = read(fd, buf, BUFF_LEN);
        if(ret == -1)
        {
            printf("errno=%d\n", errno);
            if(errno == ECONNREFUSED)
            {
                printf("server disconnect\n");
                return;
            }
        }
        else
        {
            printf("recieve server msg=%s\n",buf);
        }
        sleep(1);
    }
}

#else
void udp_msg_sender(int fd, struct sockaddr* dst)
{
    int ret = 0;
    socklen_t len;

    struct sockaddr_in src;
    while(1)
    {
        char buf[BUFF_LEN] = "TEST UDP MSG!\n";
        len = sizeof(*dst);
        printf("client:%s\n",buf);
        // 对方不可达的时候，会一直阻塞在这里
        ret = sendto(fd, buf, BUFF_LEN, 0, dst, len);
        printf("sendto ret = %d\n", ret);

        memset(buf, 0, BUFF_LEN);
        // 对方不发送时，会一直阻塞在这里
        recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&src, &len);

        printf("server:%s\n",buf);
        sleep(1);
    }
}
#endif

int main(int argc, char* argv[])
{
    int client_fd;
    struct sockaddr_in ser_addr;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    printf("server ip=%d,port=%d\n", SERVER_IP, SERVER_PORT);
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(SERVER_PORT);

    udp_msg_sender(client_fd, (struct sockaddr*)&ser_addr);

    close(client_fd);

    return 0;
}