/*
    ltserversocket.c
    write by zouql 20131117

    zouqinglei@163.com 
    All right reserved.

   
*/

#include "lt_clientsocket.h"
#include "lt_pthread.h"
#include "lt_singlethread.h"
#include "lt_list_s.h"

#define LT_MAX_REQUEST 32

typedef struct stLT_Client LT_Client;

typedef struct stLT_Request
{
    int index;
    unsigned int requestID;    /* system alloc ID for match,maybe the array index */
	int state; /* 0: free slot, 1: used */
		
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
    int nHasReply;

    LT_Client * pClient;

	void * replyData;   
}LT_Request;

typedef struct stLT_ComboData
{
    LT_Client * pClient;
    unsigned int requestID; 
    void * data; 
}LT_ComboData;


typedef struct stLT_Client
{
	/* call list */
    LT_Request * pRequestList;
    int currRequestNum;

    unsigned int seqRequestID;
	pthread_mutex_t mutex;
	
    //LT_ClientParam * param;
    char * serverIP;
    unsigned short listenPort;
    
    int mt; /* multi thread */
    int multiNum; /* mulit & syn mode, the max request num */

    int syn; /* synchronorious */

    clientParseFunc            parseFunc;
    clientParseStructAllocFunc parseStructAllocFunc;
    clientFreeDataFunc         parseStructFreeFunc;
    clientSerialFunc           serialFunc;
    clientFreeDataFunc         requestDataFreeFunc;
    clientFreeDataFunc         replyDataFreeFunc;
	
	int clientSocket; 
    int abort_loop;
	int connected;       /* if connect, then connected is 1, else is 0 */
   
    LT_Thread * pReadThread;

	int connTimeout; /* seconds */
	int sendTimeout;	 /* seconds */
	int recvTimeout;	 /* seconds */

	int autoReconnectSeconds; /* seconds */

    char buf[1024];
    void * parseStruct;

    //reply packet data list
    LT_List * pReplyList;
}LT_Client;


/*
static func
*/
static int LT_Client_read(LT_Client * pClient);
static int LT_Client_send_buf(LT_Client * pClient, void * buf, int buflen);
static int LT_Client_send_buf_mutex(LT_Client * pClient, void * buf, int buflen);
static int LT_Client_Request_signal(LT_Client * pClient, unsigned int requestID,
									void * replydata);
static int LT_Client_broadcast_error(LT_Client * pClient,int errcod);

static LT_Request * LT_Client_find_freeslot(LT_Client * pClient);
static int LT_Client_freeslot(LT_Client * pClient, LT_Request * pRequest);

static pthread_handler_decl(handle_read_socket,arg);

static int LT_Client_Reconnect(LT_Client * pClient);

static int LT_Client_Send_Inter(LT_Client * pClient, unsigned int requestID, const void * requestData);
static int LT_Client_Recv_Inter(LT_Client * pClient, unsigned int * requestID, void ** replyData, unsigned int waitSeconds);

static void LT_Client_ComboData_Free(LT_ComboData * pComboData)
{
    LT_Client * pClient;
	if(pComboData)
    {
        pClient = pComboData->pClient;
        if(pComboData->data)
        {
            pClient->replyDataFreeFunc(pComboData->data);
        }
        free(pComboData);
    }
}

int LT_Request_init(LT_Request * pRequest)
{
	if(!pRequest)
		return -1;
	pthread_mutex_init(&pRequest->mutex, NULL);
	pthread_cond_init(&pRequest->cond, NULL);

	pRequest->state = 0;
	
	return 0;
}

int LT_Request_destroy(LT_Request * pRequest)
{
	if(pRequest == NULL)
		return -1;
	
	pthread_mutex_destroy(&pRequest->mutex);
	pthread_cond_destroy(&pRequest->cond);

	pRequest->state = 0;
	
	return 0;
}



/*
 * server func 
 * need read config parameters is: port, maxclient, maxrequestsize,
 * reply threadpool max size
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
						  clientFreeDataFunc         replyDataFreeFunc)
{
    LT_Client * pClient;
    int size;
    int i;
	
	if( parseFunc == NULL || 
        parseStructAllocFunc == NULL ||
        parseStructFreeFunc == NULL ||
        serialFunc == NULL ||
        requestDataFreeFunc == NULL ||
	    replyDataFreeFunc == NULL )
	   return NULL;

    if(serverIP == NULL)
        return NULL;

    pClient = (LT_Client *)malloc(sizeof(LT_Client));
    BZERO(pClient, sizeof(LT_Client));

    pClient->serverIP = STR_NEW(serverIP);
    pClient->listenPort = listenPort;
    
    pClient->seqRequestID = 1;

    pClient->mt = mt;
    pClient->multiNum = LT_MAX_REQUEST;

    pClient->syn = syn;

    pthread_mutex_init(&pClient->mutex,NULL);
	
    pClient->parseFunc = parseFunc;
    pClient->parseStructAllocFunc = parseStructAllocFunc;
    pClient->parseStructFreeFunc = parseStructFreeFunc;
    pClient->serialFunc = serialFunc;
	pClient->requestDataFreeFunc = requestDataFreeFunc;
    pClient->replyDataFreeFunc = replyDataFreeFunc;
	
	pClient->connTimeout = 10;
	pClient->sendTimeout = 10;
	pClient->recvTimeout = 10;

    pClient->autoReconnectSeconds = autoReconnectSeconds;

    pClient->abort_loop = 0;

    

	if(pClient->mt)
	{
        size = sizeof(LT_Request) * pClient->multiNum;
	    pClient->pRequestList = (LT_Request *)malloc(size);
	    BZERO(pClient->pRequestList,size);
	    for(i = 0; i < pClient->multiNum; i++)
	    {
		    LT_Request_init(&pClient->pRequestList[i]);
	    }
        pClient->currRequestNum = 0;
        pClient->pReadThread = LT_Thread_new(handle_read_socket, pClient);
	}
   
    pClient->parseStruct = pClient->parseStructAllocFunc();

    pClient->pReplyList = LT_List_new(0);

	return pClient;
}


/*
 * connect server 
 */
int LT_Client_Start(LT_Client * pClient)
{
	struct timeval tv;
 
	if(pClient == NULL)
		return -1;
	tv.tv_sec = pClient->connTimeout;
	tv.tv_usec = 0;

	pClient->clientSocket = lt_connectwait(pClient->serverIP,
		pClient->listenPort,tv);
	if(pClient->clientSocket <= 0)
	{
		return LT_ERR_CONNECT;
	}
    lt_setblock(pClient->clientSocket,0);


	if(pClient->mt)
	{
        //start multi request read thread
		LT_Thread_start(pClient->pReadThread);
	}
	
	pClient->connected = 1;

	return 0;
}


int LT_Client_Stop(LT_Client * pClient)
{

    pClient->abort_loop = 1;
    lt_close(pClient->clientSocket);
    pClient->connected = 0;
	pClient->clientSocket = 0;
    
	if(pClient->mt)
	{
	    LT_Thread_stop(pClient->pReadThread);

	    LT_Client_broadcast_error(pClient, LT_ERR_SYSTEM);

        while(pClient->currRequestNum != 0)
        {
            SLEEP(1);
        }
	}
	
	return 0;
}

int LT_Client_Free(LT_Client * pClient)
{
    int i;
	pthread_mutex_destroy(&pClient->mutex);
	
    if(pClient->serverIP)
        free(pClient->serverIP);

    if(pClient->mt)
	{
        LT_Thread_free(pClient->pReadThread);
        for(i = 0; i < pClient->multiNum; i++)
	    {
		    LT_Request_destroy(&pClient->pRequestList[i]);
	    }
        free(pClient->pRequestList);
    }


    pClient->parseStructFreeFunc(pClient->parseStruct);

    LT_List_free(pClient->pReplyList, LT_Client_ComboData_Free);

    free(pClient);

	return 0;
}


static unsigned int LT_Client_getNextRequestID(LT_Client * pClient)
{
	unsigned int requestID;
	pthread_mutex_lock(&pClient->mutex);
	requestID = pClient->seqRequestID++;
    if(requestID == 0)
    {
        requestID = pClient->seqRequestID++;
    }
	pthread_mutex_unlock(&pClient->mutex);

	return requestID;
}


static int LT_Client_invoke_oneway(LT_Client * pClient,  const void * requestData)
{
	int ret;
	unsigned int requestID;

	requestID = LT_Client_getNextRequestID(pClient);
	
	ret = LT_Client_Send_Inter(pClient, requestID, requestData);

	return ret;
	
}



static int LT_Client_invoke_single(LT_Client * pClient,const void * requestData, void ** replyData, unsigned int waitSeconds)
{
	int ret;
    unsigned int requestID;
    unsigned int replyID;
    LT_ComboData * pComboData;

	/* send */
	ret = 0;  
    requestID = pClient->seqRequestID++;   
    ret = LT_Client_Send_Inter(pClient,requestID, requestData);
	if(ret != 0)
		return ret;

	/* recv */
    replyID = 0;

    /* first check the pReplyList */
    while((pComboData = LT_List_get(pClient->pReplyList, 0)) != NULL)
    {
        if(pComboData->requestID != requestID)
        {
            pClient->replyDataFreeFunc(pComboData->data);
            free(pComboData);
        }
        else
        {
            *replyData = pComboData->data;

            free(pComboData);
            return 0;
        }
    }

    ret = LT_Client_Recv_Inter(pClient, &replyID, replyData, waitSeconds);

    if(ret == 0)
    {
        if((replyID != 0) && (requestID != replyID))
        {
            pClient->replyDataFreeFunc(*replyData);
            return LT_ERR_FAIL;
        }
    }

    return ret;
}

static int LT_Client_invoke_single_mutex(LT_Client * pClient,const void * requestData, void ** replyData, unsigned int waitSeconds)
{
	int ret;
	pthread_mutex_lock(&pClient->mutex);
	ret = LT_Client_invoke_single(pClient, requestData,replyData,waitSeconds);
	pthread_mutex_unlock(&pClient->mutex);

	return ret;
}


static int LT_Client_invoke_multiplex(LT_Client * pClient,const void * requestData, 
					 void ** replyData,  unsigned int waitSeconds)
{
	int ret;
	char * sendbuf;
	int sendbuflen;
	unsigned int requestID;
	LT_Request * pRequest;
	struct timespec ts;

	/* prepare the send buf */
	sendbuf = NULL;
	sendbuflen = 0;
	requestID = LT_Client_getNextRequestID(pClient);

	/* before send, alloc the request slot when waitseconds > 0 */
	pRequest = LT_Client_find_freeslot(pClient);
	if(pRequest == NULL)
	{
		free(sendbuf);
		return LT_ERR_DENIED;
	}

    pRequest->requestID = requestID;
    pRequest->replyData = NULL;

	/* send */
    ret = LT_Client_Send_Inter(pClient, requestID, requestData);

	if(ret == LT_ERR_CONNECT)
	{
		pClient->connected = 0;
	}

	if(ret != 0)
	{
		/* free request slot */
		LT_Client_freeslot(pClient, pRequest);
		return ret;
	}

	/* wait reply */
	ts.tv_sec = waitSeconds;
	ts.tv_nsec = 0;
	pthread_mutex_lock(&pRequest->mutex);
	if(pRequest->replyData == NULL)
	{
		ret = my_pthread_cond_timedwait(&pRequest->cond, &pRequest->mutex, &ts);
		if(ret == 0)
		{
            if(pRequest->replyData != NULL)
            {
			    *replyData = pRequest->replyData;
            }
            else
            {
                ret = LT_ERR_SYSTEM;
            }
		}
        else if(ret == ETIMEDOUT)
        {
            ret = LT_ERR_TIMEOUT;
        }
	}
    else
    {
        *replyData = pRequest->replyData;
    }

    pRequest->requestID = 0;
	pthread_mutex_unlock(&pRequest->mutex);

	/* free request slot */
	LT_Client_freeslot(pClient, pRequest);
	
	return ret;
}

/*
    waitseconds: 0: one way
                 -1: default use client's recv_time, default 10
                 >0: use this wait seconds
*/

int LT_Client_Invoke(LT_Client * pClient,const void * requestData, void ** replyData, unsigned  int waitSeconds)
{
	if(pClient == NULL)
		return LT_ERR_CONNECT;

    if(pClient->syn == 0)
        return LT_ERR_DENIED;

    if(pClient->abort_loop == 1)
        return LT_ERR_DENIED;
	
	if(pClient->connected == 0)
		return LT_ERR_CONNECT;

	if(pClient->clientSocket == 0)
		return LT_ERR_CONNECT;

	if(waitSeconds == 0)
	{
		return LT_Client_invoke_oneway(pClient, requestData);
	}
    else if(waitSeconds == (unsigned int)(-1))
    {
        waitSeconds = 15;
    }
  

	if(pClient->mt)
	{
        return LT_Client_invoke_multiplex(pClient, requestData, replyData, waitSeconds);
	}
	else
	{
		return LT_Client_invoke_single_mutex(pClient, requestData, replyData,  waitSeconds);
	}

}

int LT_Client_Send_Inter(LT_Client * pClient, unsigned int requestID, const void * requestData)
{
    int ret;
	char * sendbuf;
	int sendbuflen;

	/* send */
	ret = 0;
	sendbuf = NULL;
	sendbuflen = 0;

	ret = pClient->serialFunc(requestID, 
                              requestData, 
                              &sendbuf, 
                              &sendbuflen);
	if(ret != 0)
		return ret;

	if(sendbuf == NULL)
	{
		return LT_ERR_BUF;
	}

	ret = LT_Client_send_buf_mutex(pClient, sendbuf,sendbuflen);
	free(sendbuf);

    return ret;

}

int LT_Client_Send(LT_Client * pClient, unsigned int requestID, const void * requestData)
{
    if(pClient == NULL)
        return LT_ERR_CONNECT;

    if(pClient->syn == 1)
        return LT_ERR_DENIED;

	return LT_Client_Send_Inter(pClient, requestID, requestData);

}

int LT_Client_Recv_Inter(LT_Client * pClient, unsigned int * requestID, void ** replyData, unsigned int waitSeconds)
{
	int ret;
    LT_ComboData * pComboData;

	struct timeval tv;
	int bytesread;

	ret = 0;

    if(pClient->mt)
    {
        pComboData = LT_List_get(pClient->pReplyList, waitSeconds);

        if(pComboData)
        {
            *requestID = pComboData->requestID;
            *replyData = pComboData->data;

            free(pComboData);
            return 0;
        }
        return LT_ERR_TIMEOUT;
    }

	/* recv */

    /* first check the pReplyList */
    pComboData = LT_List_get(pClient->pReplyList, 0);

    if(pComboData)
    {
        *requestID = pComboData->requestID;
        *replyData = pComboData->data;

        free(pComboData);
        return 0;
    }

	tv.tv_sec = waitSeconds;
	tv.tv_usec = 0;
	
	while(1)
	{
		bytesread = lt_readwait(pClient->clientSocket, pClient->buf, 1024, tv);
	
		if(bytesread > 0)
		{
            ret = pClient->parseFunc(pClient->buf, bytesread, pClient, pClient->parseStruct);
                
            /*
                parse return 0: just parse ok
                             LT_ERR_BUF: parse part, need more receive
                             LT_ERR_PACKET: the packet style error,need close the socket.
            */
            if(ret == 0)
            {
                pComboData = LT_List_get(pClient->pReplyList, 0);

                if(pComboData)
                {
                    *requestID = pComboData->requestID;
                    *replyData = pComboData->data;

                    free(pComboData);
                    return 0;
                }
                else
                {
                    return LT_ERR_FAIL;
                }
            }
            else if(ret == LT_ERR_BUF) /* the circbuf size is little a packet size */
            {
                continue;
            }
            else /* the packet is error, need disconnect*/
            {
                return LT_ERR_CONNECT;
            }
            
        }
		else if (bytesread == 0)
		{
			return LT_ERR_CONNECT;
		}
        else if(bytesread == -2)
        {
            return LT_ERR_TIMEOUT;
        }
		else if (bytesread < 0)
		{ 
			if(SOCKET_ERRNO != SOCKET_EWOULDBLOCK)
				return LT_ERR_CONNECT;
	    
			bytesread = 0;
		}
	}

	return LT_ERR_FAIL;
}

int LT_Client_Recv(LT_Client * pClient, unsigned int * requestID, void ** replyData, unsigned int waitSeconds)
{
    if(pClient == NULL)
        return LT_ERR_CONNECT;

    if(pClient->syn == 1)
        return LT_ERR_DENIED;

    return LT_Client_Recv_Inter(pClient, requestID, replyData, waitSeconds);
}

pthread_handler_decl(handle_read_socket,arg)
{
	int ret;
    int retval;
    struct timeval timeout;
    fd_set readset,exset;
    LT_Client * pClient;
	LT_Thread * pThread;
	int err = 0;

    pClient = (LT_Client *)arg;
	pThread = pClient->pReadThread;
	
	while(!pThread->abort_loop)
    {

		if(pClient->connected == 0)
		{
            if(pClient->autoReconnectSeconds > 0)
            {
			    SLEEP(pClient->autoReconnectSeconds);
			    /* reconnect */
			    LT_Client_Reconnect(pClient);
			    continue;
            }
            else
            {
                SLEEP(1);
                continue;
            }
		}

		FD_ZERO(&readset);
		FD_ZERO(&exset);
		FD_SET(pClient->clientSocket,&readset);
		FD_SET(pClient->clientSocket,&exset);
	 
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
		/* check read */
		if(FD_ISSET(pClient->clientSocket,&readset))
		{
			ret = LT_Client_read(pClient); 
			if(ret != 0) /* maybe close */
			{
				pClient->connected = 0;
				LT_Client_broadcast_error(pClient, LT_ERR_CONNECT);
			}
		}

		if(FD_ISSET(pClient->clientSocket,&exset))/* maybe close */
		{
			pClient->connected = 0;
			LT_Client_broadcast_error(pClient, LT_ERR_CONNECT);
		}
	}
	LT_Thread_exit(pThread);
}

static LT_Request * LT_Client_find_freeslot(LT_Client * pClient)
{
	int i;
	LT_Request * pRequest;
	
	pRequest = NULL;

	pthread_mutex_lock(&pClient->mutex);
	for(i = 0; i < pClient->multiNum; i++)
	{
		if(pClient->pRequestList[i].state == 0)
		{
            pRequest = &pClient->pRequestList[i];
			pRequest->state = 1;

            pClient->currRequestNum++;
			break;
		}
		
	}
    pthread_mutex_unlock(&pClient->mutex);
	
	return pRequest;
}

static int LT_Client_freeslot(LT_Client * pClient, LT_Request * pRequest)
{

    pthread_mutex_lock(&pClient->mutex);

	pRequest->state = 0;
	pRequest->nHasReply = 0;
	pRequest->requestID = 0;
    pRequest->replyData = NULL;
    
    pClient->currRequestNum--;
	pthread_mutex_unlock(&pClient->mutex);

	return 0;
}


/*
 * LT_Client_read()
 * if success return 0
 * if connect error return LT_ERR_CONNECT,
 * if packet style error then return LT_ERR_PACKET, need disconnect
 *
 */
int LT_Client_read(LT_Client * pClient)
{
    int ret;
    int bytesread;

    
#ifdef __WIN__
	bytesread = recv(pClient->clientSocket,pClient->buf,1024,0);
#else
    bytesread = read(pClient->clientSocket, pClient->buf, 1024);
#endif
    if(bytesread > 0)
    {
		
        ret = pClient->parseFunc(pClient->buf, bytesread, pClient, pClient->parseStruct);
        if(ret == 0)
        {
            return 0;	
        }
        else if(ret == LT_ERR_BUF) /* the circbuf size is little a packet size */
        {
            return 0;
        }
        else /* the packet is error, need disconnect*/
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


static int LT_Client_send_buf(LT_Client * pClient, void * buf, int buflen)
{
	int ret;
    struct timeval tv;
	tv.tv_sec = pClient->sendTimeout;
    tv.tv_usec = 0;
    
	ret = lt_writeblock(pClient->clientSocket, buf, buflen, tv);

	if(ret == -1)
		return LT_ERR_CONNECT;
	else if(ret == -2)
		return LT_ERR_TIMEOUT;

    return ret;
}


static int LT_Client_send_buf_mutex(LT_Client * pClient, void * buf, int buflen)
{
	int ret;
    
    pthread_mutex_lock(&pClient->mutex);
    ret = LT_Client_send_buf(pClient, buf, buflen);
    pthread_mutex_unlock(&pClient->mutex);
    
    return ret;
}



static int LT_Client_Request_signal(LT_Client * pClient, unsigned int requestID,
									void * replyData)
{
	int i;
	int ret;
	LT_Request * pRequest;
	
    ret = -1;
	/* search */
	for(i = 0; i < pClient->multiNum; i++)
	{
		pRequest = &pClient->pRequestList[i];
		if((pRequest->state == 1) && (pRequest->requestID == requestID))
		{
            pthread_mutex_lock(&pRequest->mutex);
            if(pRequest->requestID == requestID)
            {
			    pRequest->replyData = replyData;
                ret = 0;
            }
            pthread_mutex_unlock(&pRequest->mutex);
            pthread_cond_signal(&pRequest->cond);
            break;
		}
	}

	return ret;
}

static int LT_Client_broadcast_error(LT_Client * pClient,int errcod)
{
	/* signal all wait request */
	int i;
	LT_Request * pRequest;

	/* search */
	for(i = 0; i < pClient->multiNum; i++)
	{
		pRequest = &pClient->pRequestList[i];
		
		pthread_mutex_lock(&pRequest->mutex);
		if((pRequest->state == 1))
		{
			pthread_cond_signal(&pRequest->cond);
		}
		pthread_mutex_unlock(&pRequest->mutex);
		
	}

	return 0;
}

static int LT_Client_Reconnect(LT_Client * pClient)
{
	struct timeval tv;

    while(pClient->currRequestNum != 0)
    {
        if(pClient->abort_loop == 1)
            return 0;
        SLEEP(1);
    }
	
	lt_close(pClient->clientSocket);
	pClient->clientSocket = 0;

	tv.tv_sec = pClient->connTimeout;
	tv.tv_usec = 0;

	pClient->clientSocket = lt_connectwait(pClient->serverIP,
		pClient->listenPort,tv);

	if(pClient->clientSocket <= 0)
	{
		return LT_ERR_CONNECT;
	}
	
	lt_setblock(pClient->clientSocket,0);
	
	pClient->connected = 1;
	
	return 0;
}

/*
    当用户不需要requestID时，需要把参数requestID传0，这样，
    系统不会比对收到requestID和发送的requestID, 否则，会比对，如果比对不成功的
    话，就会认为收到的数据无效，需要再等待看看是否有数据传过来。
    具体要看用户的协议怎么定义，如果是短连接，单路请求应用，requestID参数应该不用使用。主要是考虑到特使情况下的超时，
    导致该次请求把上一次的回复当做了结果。
*/

int LT_Client_Append(LT_Client * pClient, unsigned int requestID, void * replyData)
{
    LT_ComboData * pComboData;

    if(pClient->mt && pClient->syn)
    {
        /* find request, and signal it */
		if(LT_Client_Request_signal(pClient, requestID, replyData) != 0)
		{
			pClient->replyDataFreeFunc(replyData);
		}
    }
    else
    {
        pComboData = (LT_ComboData *)malloc(sizeof(LT_ComboData));
        pComboData->requestID = requestID;
        pComboData->data = replyData;

        LT_List_put(pClient->pReplyList, pComboData);
    }

    return 0;
}