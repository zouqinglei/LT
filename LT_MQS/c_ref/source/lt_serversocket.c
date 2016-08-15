/*
    ltserversocket.c
    write by zouql 20131117
    zouqinglei@163.com 
    All right reserved.
   
*/

#include "lt_serversocket.h"
#include "lt_record.h"
#include "lt_pthread.h"
#include "lt_singlethread.h"
#include "lt_threadpool.h"
#include "lt_list_s.h"
/* inner define */

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
	
    /* request list , the struct is LT_RequestInfo */
    LT_List * pRequestList;

    /* work thread pool */
    LT_ThreadPool * pWorkThdPool;
   
}LT_Server;

/*
static func

*/
static LT_RequestInfo * LT_RequestInfo_new(LT_Server * pServer, LT_ClientNode * pClientNode);

static void LT_RequestInfo_free(LT_RequestInfo * pInfo);

static int  LT_ClientNode_live(LT_Server * pServer,  int socket);

static int LT_ClientNode_unlive(LT_ClientNode * pClient);

static int LT_ClientNode_unlive_mutex(LT_ClientNode * pClient, unsigned int clientID);

static void LT_ClientNode_destroy(LT_ClientNode * pClient);

static int LT_Server_findfreeslot(LT_Server * pServer);

static LT_ClientNode * LT_Server_findslot(LT_Server * pServer, unsigned int clientID);

int LT_Server_clientinfo(LT_Server * pServer, json_object ** record);




static pthread_handler_decl(handle_read_socket,arg);
static pthread_handler_decl(handle_work,arg);

/* set the listen and read socket set */
static int LT_Server_FD_SET(LT_Server * pServer, fd_set *pset);
static int LT_Server_check_readset(LT_Server * pServer, fd_set *pset);
static int LT_Server_check_exset(LT_Server * pServer, fd_set *pset);
static int LT_Server_Reply(LT_Server * pServer, LT_RequestInfo * pInfo);


LT_RequestInfo * LT_RequestInfo_new(LT_Server * pServer, LT_ClientNode * pClientNode)
{
    LT_RequestInfo * pInfo;

    pInfo = (LT_RequestInfo *)malloc(sizeof(LT_RequestInfo));
	BZERO(pInfo, sizeof(LT_RequestInfo));

    pInfo->clientID = pClientNode->clientID;
    pInfo->commSocket = pClientNode->commSocket;
    pInfo->local = pClientNode->local; 
    pInfo->remote = pClientNode->remote;
    pInfo->pServer = pServer;
 
    return pInfo;
}

void LT_RequestInfo_free(LT_RequestInfo * pInfo)
{
	LT_Server * pServer;
	if(pInfo)
    {
        pServer = pInfo->pServer;
        if(pInfo->requestData)
        {
            pServer->param.requestDataFreeFunc(pInfo->requestData);
        }

		if(pInfo->replyData)
        {
            pServer->param.replyDataFreeFunc(pInfo->replyData);
        }

        free(pInfo);
    }
}

/*
 * client func 
 */

int  LT_ClientNode_live(LT_Server * pServer,int socket)
{
    int ret;
    int index;
    LT_ClientNode * pClient;
    unsigned int clientID;
    LT_RequestInfo * pInfo;
    int len;
    
    index = LT_Server_findfreeslot(pServer);
    if(index < 0)
        return index;

	clientID = pServer->seqClientID++;
	if(clientID == 0)
	{
		clientID = pServer->seqClientID++;
	}

    pClient = &pServer->clientlist[index];

	pClient->clientID = clientID;
    pClient->state = LT_CLIENTNODE_STATE_LIVE;
	pClient->commSocket = socket;
    len = sizeof(pClient->local);
    ret = getsockname(pClient->commSocket, (struct sockaddr *)&pClient->local, &len);
    len = sizeof(pClient->local);
    ret = getpeername(pClient->commSocket, (struct sockaddr *)&pClient->remote, &len);

    /* parse area init callback func */
    pClient->parseStruct = pServer->param.parseStructAllocFunc();

     /* client connect event for process */
    pInfo = LT_RequestInfo_new(pServer, pClient);
    pInfo->ltEvent = LT_SERVER_EVENT_CONNECT;
    pInfo->dontReply = 1;

    /*put data to request list */
    LT_List_put(pServer->pRequestList,pInfo);

	return 0;
}

int LT_ClientNode_unlive(LT_ClientNode * pClient)
{
    LT_RequestInfo * pInfo;
    LT_Server * pServer;
    
    pServer = pClient->pServer;

	if(pClient->state == LT_CLIENTNODE_STATE_LIVE)
	{
        /* client close event */
         /* client connect event for process */
        pInfo = LT_RequestInfo_new(pServer, pClient);
        pInfo->ltEvent = LT_SERVER_EVENT_DISCONNECT;
        pInfo->dontReply = 1;

        /*put data to request list */
        LT_List_put(pServer->pRequestList,pInfo);
        

		pClient->clientID = 0;
		if(pClient->commSocket > 0)    
		{
			lt_close(pClient->commSocket);
			pClient->commSocket = 0;
		}

        /* free parse area */
        pServer->param.parseStructFreeFunc(pClient->parseStruct);
        pClient->parseStruct = NULL;

		pClient->state = LT_CLIENTNODE_STATE_FREE;
	}

    return 0;
}

int LT_ClientNode_unlive_mutex(LT_ClientNode * pClient, unsigned int clientID)
{
    LT_Server * pServer = pClient->pServer;  
   
    pthread_mutex_lock(&pServer->mutex);

    if(pClient->clientID == clientID)
    {
        LT_ClientNode_unlive(pClient);
    }
	
    pthread_mutex_unlock(&pServer->mutex);

    return 0;
}




void LT_ClientNode_destroy(LT_ClientNode * pClient)
{
    LT_ClientNode_unlive(pClient);
	
	pthread_mutex_destroy(&pClient->mutex);
        	
}

int LT_Server_findfreeslot(LT_Server * pServer)
{
    int index;
    int i;
    
    index = -1;
    for(i = 0; i < pServer->param.maxClient; i++)
    {
        if((pServer->clientlist[i].state == LT_CLIENTNODE_STATE_FREE))
        {
            index = i;
            break;
        }
    }

    return index;
}

LT_ClientNode * LT_Server_findslot(LT_Server * pServer, unsigned int clientID)
{
    LT_ClientNode * pClientNode;
    int i;

    pClientNode = NULL;
    for(i = 0; i < pServer->param.maxClient; i++)
    {
		if(pServer->clientlist[i].clientID == clientID)
        {
            pClientNode = &pServer->clientlist[i];
            break;
        }
    }

    return pClientNode;
}


int LT_Server_clientinfo(LT_Server * pServer, json_object ** record)
{
    int i;
    json_object * hRecord;
    int row;
    LT_ClientNode * pClient;
    char tmp[10];
    
    hRecord = ltr_new();
    ltr_appendcol(hRecord, "HOSTIP", 1, 1);
    ltr_appendcol(hRecord, "PORT", 1, 1);

    for(i = 0; i < pServer->param.maxClient; i++)
    {
        pClient = &pServer->clientlist[i];
        if(pClient->state == LT_CLIENTNODE_STATE_LIVE)
        {
            row = ltr_insertrow(hRecord, -1);
            
            ltr_setitem(hRecord, row, 0, inet_ntoa(pClient->remote.sin_addr)); 
            sprintf(tmp, "%d", pClient->remote.sin_port);
            ltr_setitem(hRecord, row, 1, tmp); 
        }
    }
    
    *record = hRecord;

    return 0;
}

/*
 * LT_Client_read()
 * if success return 0
 * if connect error return LT_ERR_CONNECT,
 * if packet style error then return LT_ERR_PACKET, need disconnect
 *
 */
int LT_ClientNode_read(LT_ClientNode * pClientNode)
{
    int ret;
    int bytesread;
    LT_Server * pServer;

    static char recvbuff[8092];

    pServer = pClientNode->pServer;
    
   
#ifdef __WIN__
	bytesread = recv(pClientNode->commSocket,recvbuff,8092,0);
#else
    bytesread = read(pClientNode->commSocket, recvbuff, 8092);
#endif
    if(bytesread > 0)
    {
        ret = pServer->param.parseFunc(recvbuff, bytesread,pServer,pClientNode);
        if(ret == LT_ERR_PACKET)
        {
            return LT_ERR_CONNECT;
        }
    }
    else if (bytesread == 0)
    {
        return LT_ERR_CONNECT;
    }
    else if (bytesread < 0)
    { 
        if(SOCKET_ERRNO != SOCKET_EWOULDBLOCK)
            return LT_ERR_CONNECT;
    
        bytesread = 0;
    }
   
    
    return 0;
}


/*
 * server func 
 * need read config parameters is: port, maxclient, maxrequestsize,
 * reply threadpool max size
 */
LT_Server * LT_Server_New(LT_ServerParam * param)
{
    int i;
    int size;
    LT_Server * pServer;
    LT_ClientNode * pClient;
    
    if(param == NULL)
        return NULL;

    if(param->parseFunc == NULL || 
        param->parseStructAllocFunc == NULL ||
        param->parseStructFreeFunc == NULL ||
        param->serialFunc == NULL ||
        param->requestDataFreeFunc == NULL ||
        param->replyDataFreeFunc == NULL ||
        param->processFunc == NULL)
    {
        return NULL;
    }

    pServer = (LT_Server *)malloc(sizeof(LT_Server));
    BZERO(pServer, sizeof(LT_Server));

    BCOPY(param,&pServer->param,sizeof(LT_ServerParam));

    pServer->seqClientID = 1;

    if((pServer->param.maxClient <= 0) || (pServer->param.maxClient > LT_MAX_CLIENT_NODE))
    {
        pServer->param.maxClient = LT_MAX_CLIENT_NODE;
    }
  
    size = sizeof(LT_ClientNode) * pServer->param.maxClient;
    pServer->clientlist = (LT_ClientNode *)malloc(size);
    BZERO(pServer->clientlist,size);
    for(i = 0; i < pServer->param.maxClient; i++)
    {
        pClient = &pServer->clientlist[i];
		pClient->pServer = pServer;

        pthread_mutex_init(&pClient->mutex,NULL);
    }

    pthread_mutex_init(&pServer->mutex,NULL);

    pServer->pRequestList = LT_List_new(param->maxRequestSize);

    pServer->pReadThread = LT_Thread_new(handle_read_socket, pServer);

    if(pServer->param.maxWorkThreadSize <= 0)
    {
        pServer->param.maxWorkThreadSize = 1;
    }

    pServer->pWorkThdPool = LT_ThreadPool_new(1,pServer->param.maxWorkThreadSize,
        handle_work,pServer);

   
	return pServer;
}

int LT_Server_Start(LT_Server * pServer)
{
	//setup server
	pServer->serverSocket = lt_setup_srv(pServer->param.listenPort);
	if(pServer->serverSocket == -1)
	{
		return LT_ERR_SYSTEM;
	}

	lt_setblock(pServer->serverSocket,0);

	LT_Thread_start(pServer->pReadThread);

    LT_ThreadPool_start(pServer->pWorkThdPool);

	return 0;
}

int LT_Server_Stop(LT_Server * pServer)
{
    if(pServer->serverSocket == -1)
        return 0;
	LT_Thread_stop(pServer->pReadThread);
       
	LT_ThreadPool_stop(pServer->pWorkThdPool);

	lt_close(pServer->serverSocket);

	return 0;
}

int LT_Server_Free(LT_Server * pServer)
{
    int i;
    LT_ClientNode * pClient;

    LT_Thread_free(pServer->pReadThread);

    for(i = 0; i < pServer->param.maxClient; i++)
    {
        pClient = &pServer->clientlist[i];
        LT_ClientNode_destroy(pClient);
    }

	pthread_mutex_destroy(&pServer->mutex);

    LT_List_free(pServer->pRequestList, LT_RequestInfo_free);

	LT_ThreadPool_free(pServer->pWorkThdPool);

    free(pServer->clientlist);

    free(pServer);

	return 0;
}




int LT_Server_FD_SET(LT_Server * pServer,fd_set *pset)
{
    int i;
    LT_ClientNode *pClient;
    /* listen socket */
    FD_ZERO(pset);
	FD_SET(pServer->serverSocket,pset);
    /* client socket */
    
    for(i = 0; i < pServer->param.maxClient; i++)
    {
	    pClient = (LT_ClientNode *)&pServer->clientlist[i];
	    if(pClient->state == LT_CLIENTNODE_STATE_LIVE)
	    {
		    FD_SET(pClient->commSocket,pset);
	    }
    }
   
    return 0;
}



int LT_Server_check_readset(LT_Server * pServer, fd_set *pset)
{   
    LT_ClientNode *pClient;
    int index;
	int socket;
    struct sockaddr_in clientaddr;
    int i;
	int ret;

    pthread_mutex_lock(&pServer->mutex);
    /* if have a client connect. check listen socket */
	if(FD_ISSET(pServer->serverSocket,pset))
    {
		socket = lt_acceptEx(pServer->serverSocket,&clientaddr);

        if(socket <= 0)
		{
			return LT_ERR_SYSTEM;
		}
		lt_setblock(socket,0);

        /* find a free slot in clientlist */
        
        index = LT_ClientNode_live(pServer, socket);
        if(index < 0)
        {
            lt_close(socket);
        }
       
    }
     
    
    for(i = 0; i < pServer->param.maxClient; i++)
    {
        pClient = &pServer->clientlist[i];
        if(pClient->state == LT_CLIENTNODE_STATE_LIVE)
        {
            if(FD_ISSET(pClient->commSocket,pset))
	        {
	            ret = LT_ClientNode_read(pClient); /* maybe close */
		        if(ret != 0)
		        {
                    LT_ClientNode_unlive(pClient);
		        }
            }
        }
    }
    
    pthread_mutex_unlock(&pServer->mutex);

    return 0;
}

int LT_Server_check_exset(LT_Server * pServer, fd_set *pset)
{    
    LT_ClientNode *pClient;
    int i;
     
   
    /* check client exset */
    pthread_mutex_lock(&pServer->mutex);

    for(i = 0; i < pServer->param.maxClient; i++)
    {
        pClient = &pServer->clientlist[i];
        if(pClient->state == LT_CLIENTNODE_STATE_LIVE)
        {
            if(FD_ISSET(pClient->commSocket,pset))
		    {
			    LT_ClientNode_unlive(pClient);
            }
        }
    }
    pthread_mutex_unlock(&pServer->mutex);
    
    return 0;
}


pthread_handler_decl(handle_read_socket,arg)
{
    int retval;
    struct timeval timeout;
    fd_set readset,exset;
    LT_Server * pServer;
	LT_Thread * pThread;
	int err = 0;

    pServer = (LT_Server *)arg;
	pThread = pServer->pReadThread;
	
	while(!pThread->abort_loop)
    {
		LT_Server_FD_SET(pServer, &readset);
		LT_Server_FD_SET(pServer, &exset);
	    
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		if(((retval = 
				select(FD_SETSIZE ,&readset,NULL,&exset,&timeout)) == -1)
	#ifdef _WIN32
			&& ((err = WSAGetLastError()) == WSAEINTR) )
	#else
			&& (errno == EINTR) )
	#endif
		{
			continue;
		}
		if(retval <= 0)
			continue;

		
		LT_Server_check_readset(pServer,&readset);
		LT_Server_check_exset(pServer,&exset);
	}
	LT_Thread_exit(pThread);
}



/* handle work center process the request and return by reply*/
pthread_handler_decl(handle_work,arg)
{
    LT_ThreadInfo * pThread;
	LT_Server * pServer;
	LT_RequestInfo * pInfo;
    
	pThread = (LT_ThreadInfo *)arg;
    pServer = (LT_Server *)pThread->userarg;

	pInfo = LT_List_get(pServer->pRequestList,2);
	if(pInfo == NULL)
		return;
	
	pServer->param.processFunc(pInfo);

    if(pInfo->ltEvent != LT_SERVER_EVENT_REQUEST)
    {
        LT_RequestInfo_free(pInfo);
        return;
    }

    if(pInfo->dontReply != 1)
    {
        LT_Server_Reply(pServer, pInfo);
    }
}

int LT_Server_Reply(LT_Server * pServer, LT_RequestInfo * pInfo)
{
    int ret;
    LT_ClientNode * pClientNode;   
    char * sendbuf;
    int sendlen;
   
    struct timeval tv;

    ret = 0;

	/* search clientID , if not't exist ,then free and return */
    pClientNode = LT_Server_findslot(pServer, pInfo->clientID);
    if(pClientNode == NULL)
    {
        return LT_ERR_NOENT;
    }
    if(pClientNode->state != LT_CLIENTNODE_STATE_LIVE)
    {
        return LT_ERR_CONNECT;
    }

    
    sendbuf = NULL;
    sendlen = 0;
    ret = pServer->param.serialFunc(pInfo, &sendbuf, &sendlen);

    if((ret != 0) || (sendbuf == NULL))
    {
        return LT_ERR_SNDDAT;
    }
   
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    pthread_mutex_lock(&pClientNode->mutex);
    ret = lt_writeblock(pClientNode->commSocket, sendbuf, sendlen, tv);
	pthread_mutex_unlock(&pClientNode->mutex);
    if(ret == -1) /* send failure,need disconnect */
    {
        ret = LT_ERR_CONNECT;
    }

    free(sendbuf);

    if(ret == LT_ERR_CONNECT)
    {
        LT_ClientNode_unlive(pClientNode);
    }
    else
    {
	    if(pInfo->closeClient == 1)
	    {
		    /* close socket */
            LT_ClientNode_unlive(pClientNode);
	    }
    }

    LT_RequestInfo_free(pInfo);

    return 0;
    
}



/* put the parse user data to the request list */
int LT_Server_Append(LT_Server * pServer, LT_ClientNode * pClientNode, unsigned int requestID, void * requestData)
{
    LT_RequestInfo * pInfo;

    pInfo = LT_RequestInfo_new(pServer, pClientNode);
    pInfo->ltEvent = LT_SERVER_EVENT_REQUEST;
    pInfo->requestID = requestID;
    pInfo->requestData = requestData;
    /*put data to request list */
    LT_List_put(pServer->pRequestList,pInfo);

    return 0;
}

void * LT_Server_GetParseStruct(LT_ClientNode * pClientNode)
{
    return pClientNode->parseStruct;
}

int LT_Server_Send(LT_Server * pServer, unsigned int clientID, unsigned int requestID, void * replyData)
{
    int i;
    int ret;
    LT_ClientNode * pClientNode;   

    LT_RequestInfo * pInfo;

    char * sendbuf;
    int sendlen;
   
    struct timeval tv;

    ret = 0;

    pInfo = (LT_RequestInfo *)malloc(sizeof(LT_RequestInfo));
	BZERO(pInfo, sizeof(LT_RequestInfo));

    pInfo->ltEvent = LT_SERVER_EVENT_REQUEST;
    pInfo->requestID = requestID;
    pInfo->replyData = replyData;
    pInfo->pServer = pServer;

    sendbuf = NULL;
    sendlen = 0;
    ret = pServer->param.serialFunc(pInfo, &sendbuf, &sendlen);

    if((ret != 0) || (sendbuf == NULL))
    {
        return LT_ERR_SNDDAT;
    }

    for(i = 0; i < pServer->param.maxClient; i++)
    {
        if(clientID > 0)
        {
		    if(pServer->clientlist[i].clientID != clientID)
            {
                continue;
            }
        }
        pClientNode = &pServer->clientlist[i];
       
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        pthread_mutex_lock(&pClientNode->mutex);
        ret = lt_writeblock(pClientNode->commSocket, sendbuf, sendlen, tv);
	    pthread_mutex_unlock(&pClientNode->mutex);

        if(clientID > 0)
        {
            break;
        }
    }
   
    free(sendbuf);

    LT_RequestInfo_free(pInfo);

    return 0;
}
