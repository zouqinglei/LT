/*
    lt_clientsocket.h
    write by zouql 20131120

    for manage socket client 
    
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef LT_CLIENTSOCKET_H
#define LT_CLIENTSOCKET_H

#include "lt_const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stLT_Client LT_Client;

/*
 * packet_parse_func()
 * return 0: if parse a packet to data success.
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * 
 *
 * 
 */

typedef int  (*clientParseFunc)(const char * recvBuff, int recvSize, LT_Client * pClient,
                                void * parseStruct); 

typedef void * (*clientParseStructAllocFunc)();

typedef int  (*clientSerialFunc)(const unsigned int requestID, const void * requestData, char ** buf, int * buflen);

typedef void (*clientFreeDataFunc)(void * data);



/* process_func is not used */

/*
 * 1. mt is 0 then the client just support single thread. if 1, support multithread
 * 2. autoreconnect only support mt = 1 mode
*/


LT_Client * LT_Client_New(const char * serverIP, 
                          unsigned short listenPort,						  
						  int mt,
                          int syn,
                          int autoReconnectSeconds,
                          clientParseFunc            parseFunc,
                          clientParseStructAllocFunc parseStructAllocFunc,
                          clientFreeDataFunc         parseStructFreeFunc,
                          clientSerialFunc           serialFunc,
                          clientFreeDataFunc         requestDataFreeFunc,
						  clientFreeDataFunc         replyDataFreeFunc);



/*
 * connect server 
 */
int LT_Client_Start(LT_Client * pClient);

/* LT_Client_Append 
   this function must used in clientParseFunc callback function 
*/
int LT_Client_Append(LT_Client * pClient, unsigned int requestID, void * replyData);

/*
 * waitseconds is >= 0
 * if == 0 then just one way invoke equl to asynchronous LT_Client_Send, 
 
    >0: use this wait seconds
*
 */

/* synchronous invoke
 * if waitseconds is zero, then the call is one way call.
 * if waitseconds > 0 && mt = 0, means just only one request/reply in once time, and not need start aj read thread, 
 * if waitseconds > 0 && mt = 1, then must be create a thread for read.
*/
int LT_Client_Invoke(LT_Client * pClient,const void * requestData, void ** replyData, unsigned int waitSeconds);


/*asynchronous send and recv */
int LT_Client_Send(LT_Client * pClient, unsigned int requestID, const void * requestData);
/*
 * waitseconds is >= 0
    0: check result, if have no reply data, return immediately.
    >0: use this wait seconds
*
 */

int LT_Client_Recv(LT_Client * pClient, unsigned int * requestID, void ** replyData, unsigned int waitSeconds);


int LT_Client_Stop(LT_Client * pClient);
int LT_Client_Free(LT_Client * pClient);



#ifdef __cplusplus
}
#endif

#endif /* LT_CLIENTSOCKET_H */

