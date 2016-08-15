/*
    ltserversocket.h
    write by zouql 20131113

    for manage socket client 

*/

#ifndef LT_SERVERSOCKET_DEF_H
#define LT_SERVERSOCKET_DEF_H

#include "lt_serversocket.h"
#include "lt_pthread.h"
#include "lt_singlethread.h"
#include "lt_threadpool.h"
#include "lt_list_s.h"


#define LT_MAX_CLIENT_NODE 1023


#define LT_CLIENTNODE_STATE_FREE   0
#define LT_CLIENTNODE_STATE_LIVE   1


typedef struct stLT_ClientNode
{
    unsigned int clientID;   
	int state; /* 0: free slot, 1: live */
	int commSocket;
	struct sockaddr_in local;
	struct sockaddr_in remote;

	pthread_mutex_t mutex;


    LT_Server * pServer;
    void * parseStruct;

}LT_ClientNode;


typedef struct stLT_Server
{
    LT_ClientNode * clientlist;
    unsigned int seqClientID;
	pthread_mutex_t mutex;

    LT_ServerParam param;
    
    int serverSocket; 

    LT_Thread * pReadThread;
	
    /* request list , the struct is LT_ClientData */
    LT_List * pRequestList;

	/* work thread pool */
    LT_ThreadPool * pWorkThdPool;
   
}LT_Server;

#ifdef __cplusplus
extern "C" {
#endif

LT_RequestInfo * LT_RequestInfo_new(LT_Server * pServer, LT_ClientNode * pClientNode);

void LT_RequestInfo_free(LT_RequestInfo * pInfo);

int  LT_ClientNode_live(LT_Server * pServer,  int socket);

int LT_ClientNode_unlive(LT_ClientNode * pClient);

int LT_ClientNode_unlive_mutex(LT_ClientNode * pClient, unsigned int clientID);

void LT_ClientNode_destroy(LT_ClientNode * pClient);

int LT_Server_findfreeslot(LT_Server * pServer);

LT_ClientNode * LT_Server_findslot(LT_Server * pServer, unsigned int clientID);

int LT_Server_clientinfo(LT_Server * pServer, char ** record);



#ifdef __cplusplus
}
#endif

#endif /* LT_SERVERSOCKET_DEF_H */

