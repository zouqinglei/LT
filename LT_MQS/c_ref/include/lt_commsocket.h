/*
  lt_commsocket.h: write by zouql 20040109.
   1:20040211: modified for ucLinux
   2:20131113: modified for ltqueue  
   zouqinglei@163.com 
    All right reserved.
*/

#ifndef LT_COMM_SOCKET_H__
#define LT_COMM_SOCKET_H__

#ifdef FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#if defined(_WIN32_WCE)
#define __WIN__
#include <windows.h>
#include <math.h>			/* Because of rint() */
#include <malloc.h>
#include <winsock.h>
#pragma comment(lib, "ws2.lib")

#elif defined(WIN32)
#define __WIN__
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <winsock.h>
#include <process.h>
#include <sys/types.h>
#include <sys/stat.h>
#pragma comment(lib,"ws2_32.lib")

#include "rpc.h"
#pragma comment(lib,"rpcrt4.lib")


#define ETIMEDOUT       WAIT_TIMEOUT
#define STR_NEW(s)   strcpy((char *)malloc((long)strlen(s)+1),s) 

#else
#include <signal.h>
//#include <sys/time.h>
#include <stddef.h>
//#include <unistd.h>
#include <stdlib.h>
//#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
//#include <semaphore.h> 
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <sys/ioctl.h>
//#include <netdb.h>
//#include <sys/errno.h>
//#include <arpa/inet.h>
//#include <sys/ipc.h>
//#include <sys/msg.h>
#ifdef AIX5
#include "uuid.h"
#else
#include <uuid/uuid.h>
#endif

#endif

#if defined(WIN32) || defined(_WIN32_WCE)
#include <memory.h>
#define BZERO(d,n)      memset(d,0,n)
#define BCMP(s,d,n)     memcmp(d,s,n)
#define BCOPY(s,d,n)    memcpy(d,s,n)

#define SOCKET_ERRNO	WSAGetLastError()
#define CLOSESOCKET(A)	closesocket(A)
#define SOCKET_EINTR	WSAEINTR
#define SOCKET_EAGAIN	WSAEINPROGRESS
#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#define SOCKET_ENFILE	ENFILE
#define SOCKET_EMFILE	EMFILE

#define SLEEP(n)        Sleep(n * 1000)
#define VSNPRINTF(a,b,c,d) _vsnprintf(a,b,c,d)
#define SNPRINTF(a,b,c,d) _snprintf(a,b,c,d)
#define INVALID_HANDLE NULL

#else
#define BZERO(d,n)      bzero(d,n)
#define BCMP(s,d,n)     bcmp(s,d,n)
#define BCOPY(s,d,n)    bcopy(s,d,n)

typedef int     SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERRNO	errno
#define CLOSESOCKET(A)	close(A)
#define SOCKET_EINTR	EINTR
#define SOCKET_EAGAIN	EAGAIN
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#define SOCKET_ENFILE	ENFILE
#define SOCKET_EMFILE	EMFILE
#define SLEEP(n)        sleep(n)
#define VSNPRINTF(a,b,c,d) vsnprintf(a,b,c,d)
#define SNPRINTF(a,b,c,d) snprintf(a,b,c,d)
#define INVALID_HANDLE -1
#define MAX_PATH PATH_MAX
#ifndef PATH_MAX
#define PATH_MAX 256
#endif
#endif 


#ifdef __cplusplus
extern "C" {
#endif

int lt_initial();
int lt_destroy();
int lt_setup_srv(unsigned short port);
int lt_accept(int fd,char *hostname,int hostnamesize);
int lt_acceptEx(int fd,struct sockaddr_in * pClientAddr);

int lt_connect(char *addr,unsigned short port);
int lt_connectwait(char *addr,unsigned short port,struct timeval timeout);

int lt_close(int fd);

int lt_readblock(int fd, void *buf, int size,struct timeval timeout);
int lt_read(int fd,void *buf,int size);
int lt_readwait(int fd,void *buf,int size,struct timeval timeout);
int lt_readline(int fd, void * buf, int size, struct timeval timeout);

int lt_writeblock(int fd, void *buf, int size,struct timeval timeout);
int lt_canread(int fd,struct timeval timeout);

int lt_setblock(int fd,int nBlock);
int lt_setnodelay(int fd);
int lt_setlinger( int fd, int onoff, int lingertime);

#ifdef __cplusplus
}
#endif
#endif


