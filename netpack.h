#ifndef NETPACK_H
#define NETPACK_H

#define PACK_SIZE 4 * 1024
#define PACK_ADD  1024

#ifdef DEBUG
	#define PACK_SIZE 10
	#define PACK_ADD  5
#endif

typedef struct NETPACK
{
    //缓存，解决分包的问题,可能会存储不止一个包
    char        *m_buff;           // 数据缓存，当接收到一个完整的包再放入消息队列中进行上层解析
    int          m_len;            // 当前数据缓存的大小
    int          m_tail;           // 没有数据时为0，有未处理完的数据时为最后数据的下一位
    int          m_head;
}NetPack_s, *PNetPack_s;

int addData2Pack(PNetPack_s pack, char * data, int len);

int copyPack2Data(PNetPack_s pack, char ** data, int *len);

#endif