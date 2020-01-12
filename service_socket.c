#include "common.h"
#include "service_socket.h"
#include "msg_queue.h"

int setNonBlocking(int fd)
{
    int opts;
    opts = fcntl(fd, F_GETFL);
    if(opts < 0)
    {
        printf("fcntl(fd,GETFL)");
        return FAIL;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(fd,F_SETFL,opts)<0)
    {
        printf("fcntl(fd,SETFL,opts)");
        return FAIL;
    }
    return OK;
}

void initListenSocket(int efd, short port, MyEpoll * epoll)
{
    struct sockaddr_in sin;
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    setNonBlocking(lfd);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    int opt = 1; 
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(lfd, (struct sockaddr *)&sin, sizeof(sin));

    listen(lfd, 20);
    eventSet(&(epoll->m_eventsSet[MAX_EVENTS]), lfd, acceptConnect, &(epoll->m_eventsSet[MAX_EVENTS]));
    eventAdd(efd, EPOLLIN, &(epoll->m_eventsSet[MAX_EVENTS]));

    return;
}

void acceptConnect(int lfd, int events, void *arg, void * expend)
{
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int cfd, i;
    if((cfd = accept(lfd, (struct sockaddr *)&cin, &len)) == -1)
    {
        if(errno != EAGAIN && errno != EINTR)
        {
            sleep(1);
        }
        printf("%s:accept,%s\n",__func__, strerror(errno));
        return;
    }

    MyEpoll * epoll = (MyEpoll*)expend;
    do
    {
        for(i = 0; i < MAX_EVENTS; i++)
        {
            if(epoll->m_eventsSet[i].m_status == 0)
                break;
        }

        if(i == MAX_EVENTS) // 超出连接数上限
        {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }
        if (setNonBlocking(cfd) == FAIL)
        {
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }

        // MyEvent * myEvent = (myEvent *) malloc(sizeof(MyEvent));
        
        eventSet(&(epoll->m_eventsSet[i]), cfd, recvData, &(epoll->m_eventsSet[i]));
        eventAdd(epoll->m_efd, EPOLLIN, &(epoll->m_eventsSet[i]));
    }while(0);

    printf("new connect[%s:%d],[time:%ld],pos[%d]\n",inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), epoll->m_eventsSet[i].m_last_active, i);
    return;
}

void eventSet(MyEvent *ev, int fd, callBackFunc cbk, void *arg)
{
    ev->m_fd = fd;
    ev->m_cbk = cbk;
    ev->m_events = 0;
    ev->m_arg = arg;
    ev->m_status = 0;
    // if(ev->m_len <= 0)
    // {
    //     memset(ev->m_buf, 0, sizeof(ev->m_buf));
    //     ev->m_len = 0;
    // }
    ev->m_last_active = time(NULL);
    return;
}

void eventAdd(int efd, int events, MyEvent *ev)
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

void eventDel(int efd, MyEvent *ev)
{
    struct epoll_event epv = {0, {0}};
    if(ev->m_status != 1)
        return;
    epv.data.ptr = NULL;
    ev->m_status = 0;

    // if(ev->m_buf)
    // {
    //     free(ev->m_buf);
    //     ev->m_buf = NULL;
    //     ev->m_len = 0;
    // }
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->m_fd, &epv);
    return;
}


void recvData(int fd, int events, void *arg, void * expend)
{
    MyEpoll * epoll = (MyEpoll*)expend;
    int efd = epoll->m_efd;

    MyEvent *ev = (MyEvent *)arg;

    char tmpBuff[BUFLEN] = {0};
    // int len = recv(fd, socket_data->m_data, sizeof(socket_data->m_data), 0);
    int len = recv(fd, tmpBuff, BUFLEN, 0);



    int opt_val;
    socklen_t opt_len = sizeof(opt_val);
 
    if(getsockopt(fd, SOL_SOCKET, SO_RCVBUF , &opt_val, &opt_len) < 0)
    {
        perror("fail to getsockopt");
    }
    printf("recv buff len=%d\n", opt_val);
    
    // eventDel(efd, ev);
    if (len > 0) 
    {
        printf("recieve data len=%d\n", len);
        // 正常情况需要将数据放在ev缓存上，然后解析出来一个包后插入到消息队列，实现起来有点耗时间，这里暂时不实现
        // 主要是处理黏包的情况
        PSocketData socket_data = (PSocketData)malloc(sizeof(SocketData));
        memset(socket_data, 0, sizeof(SocketData));

        socket_data->m_data = (char *)calloc(len + 1, 1);
        socket_data->m_fd = fd;
        socket_data->m_size = len;
        memcpy(socket_data->m_data, tmpBuff, len);
        InsertTail(epoll->m_queue, (void *)socket_data, 0);
    }
    else if (len == 0)
    {
        eventDel(efd, ev);
        close(ev->m_fd);
        printf("[fd=%d] closed\n", fd);
    } 
    else 
    {
        eventDel(efd, ev);
        close(ev->m_fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }   
    sleep(5);
    return;
}

void sendData(int fd, int events, void *arg, void * expend)
{
    MyEpoll * epoll = (MyEpoll*)expend;
    int efd = epoll->m_efd;

    MyEvent *ev = (MyEvent *)arg;

    char buff[] = "ni hao!\n";
    int len = send(fd, buff, sizeof(buff), 0);

    eventDel(efd, ev);

    if (len > 0) 
    {
        // printf("send[fd=%d], [%d]%s\n", fd, len, ev->m_buf);
        eventSet(ev, fd, recvData, ev);
        eventAdd(efd, EPOLLIN, ev);
    }
    else
    {
        close(ev->m_fd);
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }
    return ;
}


void * workproc(void *ptr)
{
    MyEpoll * epoll = (MyEpoll*)ptr;
    while(1)
    {
        PQNode node = PopHead(epoll->m_queue);
        if (node)
        {
            PSocketData data = (PSocketData)node->m_data;
            printf("workproc %ld read data=%s", pthread_self(), data->m_data);
            // while(1)
            {
                int len = send(data->m_fd, data->m_data, data->m_size, 0);
                if(len <= 0)
                {
                    if(errno == EAGAIN)
                    {
                    }
                    printf("send[fd=%d] error %s\n", data->m_fd, strerror(errno));
                }    
            }
            free(node);
        }
    }
    
    return NULL;
}

void mainLoop()
{
    int port = SERV_PORT;

    MyEpoll epoll;
    memset(&epoll, 0, sizeof(MyEpoll));

    epoll.m_efd = epoll_create(MAX_EVENTS + 1);

    if(epoll.m_efd <= 0)
        printf("create efd in %s err %s\n", __func__, strerror(errno));
    initListenSocket(epoll.m_efd, port, &epoll);
    
    struct epoll_event events[MAX_EVENTS + 1];
    printf("server running:port[%d]\n", port);

    epoll.m_queue = CreateQueue();

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // 分离状态
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, workproc, (void*)&epoll);
    pthread_create(&tid, &attr, workproc, (void*)&epoll);
    pthread_create(&tid, &attr, workproc, (void*)&epoll);

    int i;
    while(1)
    {
        int nfd = epoll_wait(epoll.m_efd, events, MAX_EVENTS+1, 1000);
        if (nfd < 0)
        {
            printf("epoll_wait error, exit\n");
            exit(-1);
        }
        for(i = 0; i < nfd; i++)
        {
            MyEvent *ev = (MyEvent *)events[i].data.ptr;
            if((events[i].events & EPOLLIN) &&(ev->m_events & EPOLLIN))
            {
                ev->m_cbk(ev->m_fd, events[i].events, ev->m_arg, (void *)&epoll);
            }
            if((events[i].events & EPOLLOUT) && (ev->m_events & EPOLLOUT))
            {
                ev->m_cbk(ev->m_fd, events[i].events, ev->m_arg, (void *)&epoll);
            }
        }
    }
}
