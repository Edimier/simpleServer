#include "netpack.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #define PACK_SIZE 4 * 1024
// #define PACK_ADD  1024

// typedef struct NETPACK
// {
//     //缓存，解决分包的问题
//     char        *m_buff;           // 数据缓存，当接收到一个完整的包再放入消息队列中进行上层解析
//     int          m_len;            // 当前数据缓存的大小
//     int          m_tail;           // 没有数据时为0，有未处理完的数据时为最后数据的下一位

// }NetPack_s, * PNetPack_s;

static int initPack(PNetPack_s pack)
{
	if (!pack) return 1;
	if (!pack->m_buff)
	{
		pack->m_buff = (char *)malloc(PACK_SIZE);
		pack->m_len = PACK_SIZE;
		pack->m_tail = pack->m_buff;
		pack->m_head = pack->m_buff;
		return 0;
	}
	return 2;
}

static int isLoop(PNetPack_s pack)
{
	return pack->m_head > pack->m_tail ? 1 : 0;
}

static int realocPack(PNetPack_s pack, int addLen, int flag)
{
	if (!pack) return 1;
	if (!pack->m_buff)
	{
		pack->m_buff = (char *)malloc(PACK_SIZE);
		pack->m_len = PACK_SIZE;
		pack->m_tail = pack->m_buff;
		pack->m_head = pack->m_buff;
	}
	else
	{
		int tailOffset = pack->m_tail - pack->m_buff;
		int headOffset = pack->m_head - pack->m_buff;
		char * newAddr = (char *)realloc(pack->m_buff, pack->m_len + addLen);
		if (newAddr)
		{
			pack->m_buff = newAddr;
			int oldLen = pack->m_len;
			int newLen = oldLen + addLen;
			if (flag)
			{
				int len = oldLen - headOffset;
				char * buff = pack->m_buff;
				int i = 0;
				while (i < len)
				{
					*(buff + newLen - 1 - i) = *(buff + oldLen - 1 - i);
					++i;
				}
				pack->m_head = pack->m_buff + headOffset + addLen;
			}
			else
			{
				pack->m_head = pack->m_buff + headOffset;
			}
			pack->m_tail = pack->m_buff + tailOffset;
			pack->m_len = newLen;
			
		}
		else
		{
			return 2;
		}
	}
	return 0;
}

static int getDataLen(PNetPack_s pack)
{
	if (!pack) return 0;
	if (isLoop(pack))
	{
		return pack->m_len - (pack->m_head - pack->m_tail);
	}
	else
	{
		return pack->m_tail - pack->m_head;
	}

}

int addData2Pack(PNetPack_s pack, char * data, int len)
{
	if (!pack || !data) return 1;

	if (!pack->m_buff)
	{
		initPack(pack);
	}
	// 先判断剩余的空间是否足够，不够则扩容
	int dataLen = getDataLen(pack);
	int capacity = pack->m_len;
	// 没有数据，第一次插入
	if (dataLen == 0)
	{
		// 空间不够需要扩容,扩容多使用一个字节，避免head和tail相等
		int addSize = len - capacity;
		if (addSize > 0)
		{
			realocPack(pack, addSize + 1, 0);
		}
		memcpy(pack->m_tail, data, len);
		pack->m_tail += len;
	}
	else
	{
		if (isLoop(pack))
		{
			int lessSize = capacity - dataLen;
			if (lessSize <= len)
			{
				realocPack(pack, len - lessSize + 1, 1);
			}
			memcpy(pack->m_tail, data, len);
			pack->m_tail += len;
		}
		else
		{
			// 空间不够需要扩容
			int lessSize = capacity - dataLen;
			if (lessSize < len)
			{
				realocPack(pack, len - lessSize + 1, 0);
			}
			int tailLessSize = pack->m_len - (pack->m_tail - pack->m_buff);
			// 不需要折叠存储
			if (tailLessSize >= len)
			{
				memcpy(pack->m_tail, data, len);
				pack->m_tail += len;
			}
			else
			{
				// 折叠存储
				memcpy(pack->m_tail, data, tailLessSize);
				memcpy(pack->m_buff, data + tailLessSize, len - tailLessSize);
				pack->m_tail += len - tailLessSize;
			}
		}
	}
	return 0;
}

int copyPack2Data(PNetPack_s pack, char ** data, int *len)
{
	int dataLen = getDataLen(pack);
	if (dataLen <= PACK_HEAD_SZIE) return 0;

	char head[PACK_HEAD_SZIE + 1] = { 0 };

	if (!isLoop(pack))
	{
		// 先读出来PACK_HEAD_SZIE个字节的数据长度，然后根据长度读取有效数据
		memcpy(head, pack->m_head, PACK_HEAD_SZIE);
		int headDataLen = atoi(head);
		// 存在有效数据，继续将数据读出来
		if (pack->m_head + headDataLen + PACK_HEAD_SZIE <= pack->m_tail)
		{
			*len = headDataLen;
			*data = (char *)malloc(headDataLen);
			memcpy(*data, pack->m_head + PACK_HEAD_SZIE, headDataLen);
			pack->m_head += headDataLen + PACK_HEAD_SZIE;

			// 数据全部读完了
			if (pack->m_head == pack->m_tail)
			{
				pack->m_head = pack->m_tail = pack->m_buff;
			}
			else if (pack->m_head > pack->m_tail)
			{
				printf("copyPack2Data error, %s:%d\n", __FILE__, __LINE__);
			}
			return 1;
		}
	}
	else
	{
		int i = 0;
		char * packTail = pack->m_buff + pack->m_len;
		char * curHead = pack->m_head;
		while (i < PACK_HEAD_SZIE && curHead < packTail)
		{
			head[i] = *curHead++;
			++i;
		}
		if (i < PACK_HEAD_SZIE)
		{
			curHead = pack->m_buff;
			while (i < PACK_HEAD_SZIE && curHead < packTail)
			{
				head[i] = *curHead++;
				++i;
			}
		}
		if (curHead >= packTail)
		{
			curHead = pack->m_buff;
		}
		int headDataLen = atoi(head);
		if (curHead > pack->m_tail)
		{
			*len = headDataLen;
			*data = (char *)malloc(headDataLen);
		    // 数据全部在尾部，全部读取
			if (curHead + headDataLen <= packTail)
			{
				memcpy(*data, curHead, headDataLen);
				curHead += headDataLen;
				pack->m_head = curHead;

				// 数据全部读完了
				if (pack->m_head == pack->m_tail)
				{
					pack->m_head = pack->m_tail = pack->m_buff;
				}
				else if (pack->m_head > pack->m_tail)
				{
					printf("copyPack2Data error, %s:%d\n", __FILE__, __LINE__);
				}
				return 1;
			}
			// 一部分在尾部，一部分在头部
			else
			{
				int tailDataSize = pack->m_len - (curHead - pack->m_buff);
				memcpy(*data, curHead, tailDataSize);
				curHead = pack->m_buff;
				int lessDataSize = headDataLen - tailDataSize;
				memcpy(*(data + tailDataSize + 1), curHead, lessDataSize);
				curHead += lessDataSize;

				// 数据全部读完了
				if (pack->m_head == pack->m_tail)
				{
					pack->m_head = pack->m_tail = pack->m_buff;
				}
				else if (pack->m_head > pack->m_tail)
				{
					printf("copyPack2Data error, %s:%d\n", __FILE__, __LINE__);
				}
				return 1;
			}
		}
		else
		{
			if (curHead + headDataLen <= pack->m_tail)
			{
				*len = headDataLen;
				*data = (char *)malloc(headDataLen);
				memcpy(*data, curHead, headDataLen);
				curHead += headDataLen;
				pack->m_head = curHead;
				// 数据全部读完了
				if (pack->m_head == pack->m_tail)
				{
					pack->m_head = pack->m_tail = pack->m_buff;
				}
				else if (pack->m_head > pack->m_tail)
				{
					printf("copyPack2Data error, %s:%d\n", __FILE__, __LINE__);
				}
				return 1;
			}
		}
	}
	return 0;
}

#ifdef TESTNETPACK
int main()
{
	NetPack_s netpack;
	netpack.m_buff = NULL;
	char buf[1024] = {0};
	printf("input data:");
	while( scanf("%s", buf) != EOF)
	{
		int n = 2;
        char sendBuff[1024] = {0};
        sprintf(sendBuff, "%02d%s%02d%s", strlen(buf), buf, strlen(buf) + n, buf);
		// int len = send(sockfd, sendBuff, strlen(sendBuff), 0);
		addData2Pack(&netpack, sendBuff, strlen(sendBuff) - n);

		while(1)
		{
			char * data = NULL;
			int len = 0;
			int ret = copyPack2Data(&netpack, &data, &len);
			if( ret && data && len > 0)
			{
				printf("get data:%s,len=%d\n", data, len);
			}
			else
			{
				break;
			}
		}

		printf("input data:");
		memset(buf, 0, 1024);
	}


}

#endif



