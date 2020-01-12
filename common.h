#ifndef COMMON_H
#define COMMON_H

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

#define MAX_EVENTS  1024 /*监听上限*/
#define BUFLEN      (8 * 1024)    /*缓存区大小*/
#define SERV_PORT   6666  /*端口号*/

#define FAIL		1
#define OK			0

#endif