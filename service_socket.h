#ifndef SERVICE_SOCKET_H
#define SERVICE_SOCKET_H

#include "msg_queue.h"
typedef void (*callBackFunc)(int fd, int events, void * arg, void * expend);

/*描述就绪文件描述符的相关信息*/
typedef struct MYEVENT_S
{
    int				m_fd;             //要监听的文件描述符
    int				m_events;         //对应的监听事件，EPOLLIN和EPLLOUT
    void		   *m_arg;            //指向自己结构体指针
    callBackFunc	m_cbk;
    int 			m_status;         //是否在监听:1->在红黑树上(监听), 0->不在(不监听)

    // 循环复用缓存，总长度理论上是接收到的最大数据长度
    // char 		   *m_buff;           // 数据缓存，当接收到一个完整的包再放入消息队列中进行上层解析
    // int 			m_len;            // 当前数据缓存的大小
    // int             m_tail;           // 有效数据的末尾后一个字节位置
    // int             m_head;           // 未处理的缓存第一个字节位置

    time_t 			m_last_active;    //记录每次加入红黑树 g_efd 的时间值
}MyEvent, * PMyEvent;

typedef struct SOCKET_DATA
{
    int      m_fd;
    char    *m_data;
    int      m_size; 
}SocketData, * PSocketData;

typedef struct EPOLL_S
{
	int         	m_efd;
	MyEvent     	m_eventsSet[MAX_EVENTS + 1];     //自定义结构体类型数组. +1-->listen fd
    PQueue          m_queue;
}MyEpoll, * PMyEpoll;

int setNonBlocking(int fd);

void initListenSocket(int efd, short port, MyEpoll * epoll);

void acceptConnect(int lfd, int events, void *arg, void * expend);

void eventSet(MyEvent *ev, int fd, callBackFunc cbk, void *arg);

void eventAdd(int efd, int events, MyEvent *ev);

void eventDel(int efd, MyEvent *ev);

void recvData(int fd, int events, void *arg, void * expend);

void sendData(int fd, int events, void *arg, void * expend);

void mainLoop();

#endif