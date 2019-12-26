#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H
#include "common.h"
// 双向队列，同时也可以作为栈和链表来使用
typedef struct QNODE
{
	void * 			m_data; //queue不负责释放此指针
	int  			m_size;   // 存储数据时m_size为数据大小
	struct QNODE * 	m_next;
	struct QNODE * 	m_pre;
}QNode, * PQNode;

typedef struct QUEUE
{
	PQNode 			m_head;
	PQNode 			m_tail;
	int 			m_count;
	pthread_mutex_t m_mutex;
	pthread_cond_t  m_cond;
}Queue, * PQueue;

PQNode  CreateQueueNode(void *,int);
int    	ReleaseQueueNode(PQueue, PQNode);

PQueue 	CreateQueue();
PQNode 	PopHead(PQueue);
PQNode 	PopTail(PQueue);
PQNode 	TopHead(PQueue);
PQNode 	TopTail(PQueue);
int    	IsQueueEmpty(PQueue);
int		InsertTail(PQueue,void *, int);
int 	InsertHead(PQueue,void *, int);
int 	ReleaseQueue(PQueue);

#endif