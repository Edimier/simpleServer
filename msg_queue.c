#include "common.h"
#include "msg_queue.h"

PQNode CreateQueueNode(void * data, int size)
{
	if(!data) return NULL;
	PQNode node = (PQNode)malloc(sizeof(QNode));
	node->m_data = data;
	node->m_size = size;
	node->m_next = NULL;
	node->m_pre = NULL;
	return node;
}

PQueue CreateQueue()
{
	PQueue queue = (PQueue)malloc(sizeof(Queue));
	queue->m_head = NULL;
	queue->m_tail = NULL;
	queue->m_count = 0;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_NORMAL);
	pthread_mutex_init(&queue->m_mutex, &mutexattr);
	pthread_cond_init(&queue->m_cond, NULL);

	return queue;
}

PQNode PopHead(PQueue queue)
{
	if( ! queue || ! queue->m_head) return NULL;
	pthread_mutex_lock(& queue->m_mutex);
	PQNode head = queue->m_head;
	queue->m_head = head->m_next;
	queue->m_count--;
	pthread_mutex_unlock(& queue->m_mutex);
	return head;
}

PQNode 	PopTail(PQueue queue)
{
	if( ! queue || ! queue->m_tail) return NULL;
	pthread_mutex_lock(& queue->m_mutex);
	PQNode tail = queue->m_tail;
	queue->m_tail = tail->m_pre;
	queue->m_count--;
	pthread_mutex_unlock(& queue->m_mutex);
	return tail;
}
PQNode 	TopHead(PQueue queue)
{
	if( ! queue || ! queue->m_head) return NULL;
	return queue->m_head;
}

PQNode 	TopTail(PQueue queue)
{
	if( ! queue || ! queue->m_tail) return NULL;
	return queue->m_tail;
}

int    	ReleaseQueueNode(PQueue queue, PQNode node)
{
	if(!node || !queue) return 0;
	pthread_mutex_lock(& queue->m_mutex);
	if(queue->m_head == node)
	{
		PopHead(queue);
	}
	else if(queue->m_tail == node)
	{
		PopTail(queue);
	}
	else
	{
		PQNode preNode = node->m_pre;
		PQNode nextNode = node->m_next;
		preNode->m_next = nextNode;
		nextNode->m_pre = preNode;
		queue->m_count--;
	}
	free(node);
	pthread_mutex_unlock(& queue->m_mutex);
	return 1;
}

int    	IsQueueEmpty(PQueue queue)
{
	if(queue)
		return queue->m_count ? 0 : 1;
	return 1;
}

int		InsertTail(PQueue queue, void * data, int size)
{
	if(!queue) return 0;
	pthread_mutex_lock(& queue->m_mutex);
	PQNode node = CreateQueueNode(data, size);
	if(IsQueueEmpty(queue))
	{
		queue->m_head = node;
		queue->m_tail = node;
		queue->m_count++;
	}
	else
	{
		PQNode tail = queue->m_tail;
		tail->m_next = node;
		node->m_pre = tail;
		queue->m_tail = node;
		queue->m_count++;
	}

	pthread_cond_signal(& queue->m_cond);
	pthread_mutex_unlock(& queue->m_mutex);
	return 1;
}
int 	InsertHead(PQueue queue, void * data, int size)
{
	if(!queue) return 0;
	pthread_mutex_lock(& queue->m_mutex);
	PQNode node = CreateQueueNode(data, size);
	if(IsQueueEmpty(queue))
	{
		queue->m_head = node;
		queue->m_tail = node;
		queue->m_count++;
	}
	else
	{
		PQNode head = queue->m_head;
		head->m_pre = node;
		node->m_next = head;
		queue->m_head = node;
		queue->m_count++;
	}
	pthread_cond_signal(& queue->m_cond);
	pthread_mutex_unlock(& queue->m_mutex);
	return 1;
}

int 	ReleaseQueue(PQueue queue)
{
	while(!IsQueueEmpty(queue))
	{
		PQNode node = queue->m_head;
		ReleaseQueueNode(queue, node);
	}
}

