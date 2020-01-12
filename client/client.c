#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#define SERV_PORT   6666  /*端口号*/
#define SERV_IP     "127.0.0.1"


typedef void (*callBackFunc)(int fd, int efd, void *arg);

/*描述就绪文件描述符的相关信息*/
typedef struct MYEVENT_ST
{
    int				m_fd;             //要监听的文件描述符
    int				m_events;         //对应的监听事件，EPOLLIN和EPLLOUT
    void		   *m_arg;            //指向自己结构体指针
    callBackFunc	m_cbk;
    int 			m_status;         //是否在监听:1->在红黑树上(监听), 0->不在(不监听)
    time_t 			m_last_active;    //记录每次加入红黑树 g_efd 的时间值
}MyEvent_s, * PMyEvent_s;

#define BUFSIZE 1024

int setNonBlocking(int fd)
{
    int opts;
    opts = fcntl(fd, F_GETFL);
    if(opts < 0)
    {
        printf("fcntl(fd,GETFL)");
        return 0;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(fd,F_SETFL,opts)<0)
    {
        printf("fcntl(fd,SETFL,opts)");
        return 0;
    }
    return 1;
}

void eventDel(int efd, MyEvent_s *ev)
{
    struct epoll_event epv = {0, {0}};
    if(ev->m_status != 1)
        return;
    epv.data.ptr = NULL;
    ev->m_status = 0;
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->m_fd, &epv);
    return;
}

void recvData(int fd, int efd, void *arg)
{
	MyEvent_s *ev = (MyEvent_s *)arg;
    char tmpBuff[BUFSIZE] = {0};
    int len = recv(fd, tmpBuff, BUFSIZE, 0);
    if (len > 0) 
    {
    	printf("recv from server:%s\n", tmpBuff);
    }
    else if (len == 0)
    {
    	eventDel(efd, ev);
        close(fd);
        printf("[fd=%d] closed\n", fd);
    } 
    else 
    {
    	eventDel(efd, ev);
        close(fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }   
    return;
}

void eventAdd(int efd, int events, MyEvent_s *ev)
{
    struct epoll_event epv={0, {0}};
    int op = 0;
    epv.data.ptr = ev;
    epv.events = ev->m_events = events;
    if(ev->m_status == 0)
    {
        op = EPOLL_CTL_ADD;
        ev->m_status = 1;
    }
    if(epoll_ctl(efd, op, ev->m_fd, &epv) < 0)
        printf("event add failed [fd=%d],events[%d]\n", ev->m_fd, events);
    else
        printf("event add OK [fd=%d],events[%0X]\n", ev->m_fd, events);
    return;
}

void eventSet(MyEvent_s *ev, int fd, callBackFunc cbk, void *arg)
{
    ev->m_fd = fd;
    ev->m_cbk = cbk;
    ev->m_events = 0;
    ev->m_arg = arg;
    ev->m_status = 0;
    ev->m_last_active = time(NULL);
    return;
}

void * workproc(void *ptr)
{
	int sockfd = *(int *)ptr;


	int opt_val;
    socklen_t opt_len = sizeof(opt_val);
 
    if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF  , &opt_val, &opt_len) < 0)
    {
        perror("fail to getsockopt");
    }
    printf("send buff len=%d\n", opt_val);


	char buf[1024] = {0};

	printf("input data:");
	while( scanf("%s", buf) != EOF)
	{
		printf("input data:");
		// while(1)
		{
            char sendBuff[1024] = {0};

            sprintf(sendBuff, "%02d%s", strlen(buf), buf);
			int len = send(sockfd, sendBuff, strlen(sendBuff), 0);
			// int len = send(sockfd, buf, , 0);
			if (len <= 0)
			{
				if(EAGAIN == errno)
				{
					printf("errno=%d\n", errno);
					sleep(1);
				}
				printf("send failed\n");
				// return NULL;
			}
			else
			{
				printf("send data len = %d\n", len);
			}
		}
		
		// int len = send(sockfd, buf, strlen(buf), 0);
		// if (len <= 0)
		// {
		// 	printf("errno=%s\n", errno);
		// 	printf("send failed\n");
		// 	return NULL;
		// }
	}
	return NULL;
}

int main(int argc, char * argv[])
{
    struct sockaddr_in server_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
    	printf("socket error\n");
    	exit(0);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    bzero(&(server_addr.sin_zero), 8);
    
    if(connect(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr)) == -1)
    {
    	printf("connect error\n");
    	exit(0);
    }

    setNonBlocking(sockfd);

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // 分离状态
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, workproc, (void*)&sockfd);

    MyEvent_s myevent;
    int efd = epoll_create(1);
    struct epoll_event events[1];
    eventSet(&myevent, sockfd, recvData, &myevent);
	eventAdd(efd, EPOLLIN, &myevent);

	int i = 0;
	while(1)
	{
		int nfd = epoll_wait(efd, events, 1, 1);

		if (nfd < 0)
	    {
	        printf("epoll_wait error, exit\n");
	        exit(-1);
	    }
	    for(i = 0; i < nfd; i++)
	    {
	        MyEvent_s *ev = (MyEvent_s *)events[i].data.ptr;
	        if((events[i].events & EPOLLIN) &&(ev->m_events & EPOLLIN))
	        {
	        	printf("sockfd=%d efd=%d m_fd=%d\n", sockfd, efd, ev->m_fd);
	            ev->m_cbk(ev->m_fd, efd, (void *)ev);
	        }
	        // if((events[i].events & EPOLLOUT) && (ev->m_events & EPOLLOUT))
	        // {
	        //     ev->m_cbk(ev->m_fd, events[i].events, ev->m_arg, (void *)&epoll);
	        // }
	    }

	}
	return 0;
}