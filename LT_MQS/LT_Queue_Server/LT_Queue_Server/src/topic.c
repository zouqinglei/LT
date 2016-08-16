/*
    topic.c
    write by zouql 20081107
*/

#include "e_global.h"
#include "topic.h"
#include "queue.h"

extern HdmHandle serverHandle;

HDTopic *HDTopic_new(void *pQueueI)
{
    HDTopic *pTopic;
    pTopic = E_NEW(HDTopic);
    if(pTopic)
    {
        pTopic->pQueueI = pQueueI;
        pTopic->pClientIDArray = eIDArray_new();
        pthread_mutex_init(&pTopic->mutex,NULL);
        pthread_cond_init(&pTopic->cond,NULL);
        pTopic->pDispatchThread = eThread_new(0,handle_topic_dispatch,pTopic);
    }

    return pTopic;
}

int HDTopic_free(HDTopic *pTopic)
{
    if(!pTopic)
        return EEPARAM;
    eThread_free(pTopic->pDispatchThread);
    eIDArray_free(pTopic->pClientIDArray);
    pthread_mutex_destroy(&pTopic->mutex);
    pthread_cond_destroy(&pTopic->cond);

    return 0;
}

int HDTopic_addclient(HDTopic *pTopic,int clientID)
{
    if(!pTopic)
        return EEPARAM;
     if(eIDArray_isset(pTopic->pClientIDArray,clientID) == 0)
            eIDArray_set(pTopic->pClientIDArray,clientID);
     return 0;
}

int HDTopic_delclient(HDTopic *pTopic,int clientID)
{
    if(!pTopic)
        return EEPARAM;
    eIDArray_clr(pTopic->pClientIDArray,clientID);
    return 0;
}

int HDTopic_signal(HDTopic *pTopic)
{
    pthread_cond_signal(&pTopic->cond);
    return 0;
}

pthread_handler_decl(handle_topic_dispatch,arg)
{
    HDTopic *pTopic;
    int i;
    int clientID;
    struct timespec ts;
    int ret;
    int size;
    HDQMsgGetOption getoption;
    HDGetWait getwait;
    HDQMsg *pQMsg;
    HdmBuf eventbuf;

    pTopic = (HDTopic *)arg;

    pthread_mutex_lock(&pTopic->mutex);
    size = HDQueueI_QueueLenght(pTopic->pQueueI);

    if(size == 0)
    {
        ts.tv_nsec = 0;
        ts.tv_sec = 5;
        my_pthread_cond_timedwait(&pTopic->cond,&pTopic->mutex,&ts);
        return;
    }
    
    getoption.Options = HDMQ_GETOPT_NO_WAIT | HDMQ_GETOPT_ACCEPT_TRUNCATED_MSG;
    getoption.MatchOptions = HDMQ_GETOPT_MATCH_NONE;

    getwait.MatchOptions = HDMQ_GETOPT_MATCH_NONE;
    getwait.clientbuflen = 1024;
    pQMsg = NULL;

    ret = HDQueueI_getMsg(pTopic->pQueueI,&getoption,&getwait,&pQMsg);
    
    if(ret !=0 )
        return;
    
    eventbuf = hd_alloc();
    hd_begin_put(eventbuf);
    hd_putblob(eventbuf,pQMsg->blob,pQMsg->size);
    hd_end_put(eventbuf);
    for(i = eIDArray_count(pTopic->pClientIDArray) -1; i >= 0; i--)
    {
        clientID = eIDArray_get(pTopic->pClientIDArray,i);
        ret = hd_server_putevent(serverHandle,clientID,((HDQueueI *)pTopic->pQueueI)->Queue.name,eventbuf,0);
        if(ret != 0)
            eIDArray_clr(pTopic->pClientIDArray,clientID);
    }
    hd_free(eventbuf);
    HDQMsg_free(pQMsg);

    pthread_mutex_unlock(&pTopic->mutex);
}

