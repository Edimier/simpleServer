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
	if(!pack) return 1;
	if(!pack->m_buff)
	{
		pack->m_buff = (char *)malloc(PACK_SIZE);
		pack->m_len = PACK_SIZE;
		pack->m_tail = 0;
		pack->m_head = 0;
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
	if(!pack) return 1;
	if(!pack->m_buff)
	{
		pack->m_buff = (char *)malloc(PACK_SIZE);
		pack->m_len = PACK_SIZE;
		pack->m_tail = 0;
		pack->m_head = 0;
	}
	else
	{
		char * newAddr = (char *)realloc(pack->m_buff, pack->m_len + addLen);
		if(newAddr)
		{
			pack->m_buff = newAddr;
			int oldLen = pack->m_len;
			int newLen = oldLen + addLen;
			if(flag)
			{
				int len = oldLen - pack->m_head;
				char * buff = pack->m_buff;
				int i = 0;
				while( i++ < len)
				{
					*(buff + newLen - 1 - i) = *(buff + oldLen - 1 - i);
				}
				pack->m_head += addLen;
			}
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
	if(!pack) return 0;
	if(isLoop(pack))
	{
		return pack->m_tail + pack->m_len - pack->m_head - 1;
	}
	else
	{
		return pack->m_tail - pack->m_head;
	}

}

int addData2Pack(PNetPack_s pack, char * data, int len)
{
	if(!pack || !data) return 1;

	if(!pack->m_buff)
	{
		initPack(pack);
	}
	// 先判断剩余的空间是否足够，不够则扩容
	int less = pack->m_len;
	if(pack->m_head != pack->m_tail)
	{
		if(isLoop(pack))
		{
			less = pack->m_head - pack->m_tail;
		}
		else
		{
			less = pack->m_len - pack->m_tail + pack->m_head;
		}
	}

	if(less < len)
	{
		if(isLoop(pack))
		{
			realocPack(pack, len - less, 1);
		}
		else
		{
			realocPack(pack, len - less, 0);
		}
	}

	if(isLoop(pack))
	{
		memcpy(pack->m_buff + pack->m_tail, data, len);
		pack->m_tail += len;
	}
	else
	{
		if(pack->m_len - pack->m_tail >= len)
		{
			memcpy(pack->m_buff + pack->m_tail, data, len);
			pack->m_tail += len;
		}
		else
		{
			int tailLen = pack->m_len - pack->m_tail;
			memcpy(pack->m_buff + pack->m_tail, data, tailLen);
			memcpy(pack->m_buff, data + tailLen, len - tailLen);
			pack->m_tail = len - tailLen;
		}
	}
	return 0;
}

int copyPack2Data(PNetPack_s pack, char ** data, int *len)
{
	char head[3] = {0};
	int dataLen = getDataLen(pack) - 2;
	if(dataLen < 2) return 0;
	
	int headTmp = pack->m_head;
	if(isLoop(pack))
	{
		int i = 0;
		while(headTmp < pack->m_len && i < 2)
		{
			*(head + i) = *(pack->m_buff + headTmp + i);
			++i;
			++headTmp;
		}

		// 头字节循环存储在了m_buff的头部
		while(i < 2)
		{
			*(head + i) = *(pack->m_buff + 1 - i);
			++i;
			++headTmp;
		}
		int headDataLen = atoi(head);
		if(headDataLen == dataLen)
		{
			*len = headDataLen;
			*data = (char *)malloc(headDataLen);

			int tailLen = pack->m_len - headTmp;

			if(tailLen > 0)
			{
				if(tailLen < headDataLen)
				{
					memcpy(*data, pack->m_buff + headTmp, tailLen);
					memcpy((*data) + tailLen, pack->m_buff, headDataLen - tailLen);
					pack->m_head = headDataLen - tailLen;
				}
				else
				{
					memcpy(*data, pack->m_buff + headTmp, headDataLen);
					pack->m_head = headTmp + headDataLen;
				}
			}
			else if(tailLen == 0)
			{
				memcpy(*data, pack->m_buff, headDataLen);
				pack->m_head = headDataLen;
			}
			else
			{
				memcpy(*data, pack->m_buff - tailLen, headDataLen);
				pack->m_head = headDataLen - tailLen;
			}
			if(pack->m_head >= pack->m_len)
			{
				pack->m_head = 0;
			}
			return 1;
		}
		else if(headDataLen > dataLen)
		{
			printf("pack data error:%s %d\n", __FILE__, __LINE__);
		}
	}
	else
	{
		memcpy(head, pack->m_buff + pack->m_head, 2);
		int headDataLen = atoi(head);

		if(headDataLen == dataLen)
		{
			*len = headDataLen;
			*data = (char *)malloc(headDataLen);
			headTmp += 2;
			memcpy(*data, pack->m_buff + headTmp, headDataLen);

			pack->m_head = headTmp + headDataLen;

			if(pack->m_head >= pack->m_len)
			{
				pack->m_head = 0;
			}
			return 1;
		}
		else if(headDataLen > dataLen)
		{
			printf("pack data error:%s %d\n", __FILE__, __LINE__);
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
        char sendBuff[1024] = {0};
        sprintf(sendBuff, "%02d%s", strlen(buf), buf);
		// int len = send(sockfd, sendBuff, strlen(sendBuff), 0);
		addData2Pack(&netpack, sendBuff, strlen(sendBuff));

		char * data = NULL;
		int len = 0;
		if(copyPack2Data(&netpack, &data, &len))
		{
			printf("get data:%s,len=%d\n", data, len);
		}

		printf("input data:");
	}


}

#endif



