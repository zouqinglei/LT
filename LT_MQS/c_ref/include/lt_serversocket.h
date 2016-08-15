/*
    ltserversocket.h
    write by zouql 20131113

    for manage socket client 
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef LT_SERVERSOCKET_H
#define LT_SERVERSOCKET_H

#include "lt_const.h"
#include "lt_commsocket.h"


#define LT_SERVER_EVENT_CONNECT 0
#define LT_SERVER_EVENT_DISCONNECT 1
#define LT_SERVER_EVENT_REQUEST 2


typedef struct stLT_Server LT_Server;
typedef struct stLT_ClientNode LT_ClientNode;

typedef struct stLT_RequestInfo
{  
    int ltEvent;

    unsigned int clientID;
	int commSocket;
	struct sockaddr_in local;
	struct sockaddr_in remote;
    LT_Server * pServer;

    unsigned int requestID;
    int    dontReply;
    int    closeClient;   
    void * requestData;
    void * replyData;
    
}LT_RequestInfo;




/*
 * server_packet_parse_func()
 * return 1: active packet
 *        0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */
typedef int  (*serverParseFunc)(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode);


typedef void * (*serverParseStructAllocFunc)();

/*
 * server_packet_packet_func()
 *
 * return 0: if packet success
 * else is error
 */

typedef int  (*serverSerialFunc)(LT_RequestInfo * pInfo, char ** buf, int * buflen);

/*
 * server_free_data_func()
 *
 * 
 */

typedef void (*serverFreeDataFunc)(void * data);


/*
 * server_process_func()
 * process the pInfo->requestdata and reply to the replydata
 */
typedef void (*serverProcessFunc)(LT_RequestInfo * pInfo);



typedef struct stLT_ServerParam
{
    unsigned short listenPort;
    int maxClient;    /* max connect client, if 0, default LT_MAX_CLIENT_NODE */
    int maxRequestSize;  /* request queue max size, if 0, default max_int */
    int maxWorkThreadSize;  /* work thread number, if 0, default 1 thread */

    serverParseFunc             parseFunc;
    serverParseStructAllocFunc  parseStructAllocFunc;
    serverFreeDataFunc          parseStructFreeFunc;
    serverSerialFunc            serialFunc;
    serverFreeDataFunc          requestDataFreeFunc;
    serverFreeDataFunc          replyDataFreeFunc;
    serverProcessFunc           processFunc;
}LT_ServerParam;

#ifdef __cplusplus
extern "C" {
#endif


LT_Server * LT_Server_New(LT_ServerParam * param);
int LT_Server_Start(LT_Server * pServer);
int LT_Server_Stop(LT_Server * pServer);
int LT_Server_Free(LT_Server * pServer);


/* put the parse user data to the request list */
int LT_Server_Append(LT_Server * pServer, LT_ClientNode * pClientNode, unsigned int requestID,  void * requestData);
void * LT_Server_GetParseStruct(LT_ClientNode * pClientNode);

int LT_Server_Send(LT_Server * pServer, unsigned int clientID, unsigned int requestID, void * replyData);


#ifdef __cplusplus
}
#endif

#endif /* LT_SERVERSOCKET_H */

