/*
    topic.c
    write by zouql 20081107
*/

#include "e_global.h"
#include "forward.h"
#include "queue.h"
#include "e_log.h"

#include "channel.h"
extern HdmHandle serverHandle;
extern sqlite3 *db;   /* sqltie3 db interface */
#define   EPSINON     0.0001       /*This   is   the   precision*/  


HDForwardDst *HDForwardDst_new(char *host,char *queue,int weight)
{
    HDForwardDst *pDst;
    pDst = E_NEW(HDForwardDst);
    if(pDst)
    {
        strcpy(pDst->host,host);
        strcpy(pDst->queue,queue);
        pDst->weight = weight;
    }

    return pDst;
}

int HDForwardDst_free(HDForwardDst *pDst)
{
    if(pDst)
        FREE(pDst);
    return 0;
}

HDForward *HDForward_new(void *pQueueI)
{
    HDForward *pForward;
    pForward = E_NEW(HDForward);
    if(pForward)
    {
        pForward->pQueueI = pQueueI;
        pForward->pDstList = linkedlist_new(0);
        /* read from SYS_FORWARD_INFO table */
        HDForward_buildFromDB(pForward);

        pthread_mutex_init(&pForward->mutex,NULL);
        pthread_cond_init(&pForward->cond,NULL);
        pForward->pDispatchThread = eThread_new(0,handle_forward_dispatch,pForward);
    }

    return pForward;
}

int HDForward_buildFromDB(HDForward *pForward)
{
    int ret;
	int result;
	char sql[1024];
    sqlite3_stmt *stmt;
    HDForwardDst *pDst;
    char *dstHost;
    char *dstQueue;
    int weight;
    HDQueueI *pQueueI;

    ret = EEFAIL;
    pQueueI = (HDQueueI *)pForward->pQueueI;
	sprintf(sql,"SELECT \
		DSTHOST,DSTQUEUE,WEIGHT \
		FROM SYS_FORWARD_INFO WHERE FORWARD_QUEUE = '%s' ",pQueueI->Queue.name);
	
    result = sqlite3_prepare(db,sql,-1,&stmt,NULL);
	if(result == SQLITE_OK)
	{
		ret = 0;
		result = sqlite3_step(stmt);
		while(result == SQLITE_ROW)
		{
            dstHost = (char *)sqlite3_column_text(stmt,0);
            dstQueue = (char *)sqlite3_column_text(stmt,1);
            weight = sqlite3_column_int(stmt,2);
            pDst = HDForwardDst_new(dstHost,dstQueue,weight);
            linkedlist_add(pForward->pDstList,pDst);

			result = sqlite3_step(stmt);
		}
	}
	
	sqlite3_finalize(stmt);

    return ret;
}

int HDForward_free(HDForward *pForward)
{
    if(!pForward)
        return EEPARAM;
    eThread_free(pForward->pDispatchThread);
    pthread_mutex_destroy(&pForward->mutex);
    pthread_cond_destroy(&pForward->cond);
    linkedlist_del(pForward->pDstList,(void *)HDForwardDst_free);

    return 0;
}


int HDForward_signal(HDForward *pForward)
{
    pthread_cond_signal(&pForward->cond);
    return 0;
}

pthread_handler_decl(handle_forward_dispatch,arg)
{
    HDForward *pForward;
    struct timespec ts;
    int ret;
    int size;
    HDQMsgGetOption getoption;
    HDGetWait getwait;
    HDQMsg *pQMsg;
    HDQueueI *pQueueI;

    HDForwardDst *pDst;
    iter_t iter;
    
    float f,f1;
    HDForwardDst *p1;
    int flag;

    pForward = (HDForward *)arg;

    /* link the dst hdmq queue */
    pQueueI = (HDQueueI *)pForward->pQueueI;

    pthread_mutex_lock(&pForward->mutex);
    size = HDQueueI_QueueLenght(pForward->pQueueI);

    if(size == 0)
    {
        ts.tv_nsec = 0;
        ts.tv_sec = 5;
        my_pthread_cond_timedwait(&pForward->cond,&pForward->mutex,&ts);
        pthread_mutex_unlock(&pForward->mutex);
        return;
    }
    
    getoption.Options = HDMQ_GETOPT_NO_WAIT |HDMQ_GETOPT_BROWSE| HDMQ_GETOPT_ACCEPT_TRUNCATED_MSG;
    getoption.MatchOptions = HDMQ_GETOPT_MATCH_NONE;

    getwait.MatchOptions = HDMQ_GETOPT_MATCH_NONE;
    getwait.clientbuflen = 4194304;
    pQMsg = NULL;

    ret = HDQueueI_getMsg(pForward->pQueueI,&getoption,&getwait,&pQMsg);
    
    if(ret != 0)
    {
        pthread_mutex_unlock(&pForward->mutex);
        return;
    }
    

    /* calc */
    /* calcu the weight to forward,num/weight is small */
    f = 0.0;
    p1 = NULL;
    flag = 0;
    linkedlist_iterate(pForward->pDstList,&iter);
    while((pDst = (HDForwardDst *)linkedlist_next(pForward->pDstList,&iter)) != NULL)
    {
        if(HDChannelPool_checkLink(pDst->host) < 0)
        {
            continue;
        }
        if(pDst->weight < 0)
        {
            ret = HDChannelPool_putmsg(pDst->host,pDst->queue,pQMsg);
            if(ret == 0)
                flag = 1;  
            continue;
        }
        else if(pDst->weight == 0)
            continue;
        
        f1 = (float)(pDst->num / pDst->weight);
        if(fabs(f1 - 0.0) < EPSINON)
        {
            p1 = pDst;
            break;
        }
        
        if(p1 == NULL)
        {
            p1 = pDst;
            f = f1;
        }
        else if(fabs(f1 - f) < EPSINON)
        {
            p1 = pDst;
            f = f1;
        }
    }
    
    if(p1)
    {
        ret = HDChannelPool_putmsg(p1->host,p1->queue,pQMsg);
        if(ret == 0)
        {
            flag = 1;
            p1->num++;
        }
    }

    /* success,delete from queue */
    if(flag == 1)
    {
        getoption.Options = HDMQ_GETOPT_NO_WAIT | HDMQ_GETOPT_ACCEPT_TRUNCATED_MSG;
        getoption.MatchOptions = HDMQ_GETOPT_MATCH_NONE;
        
        getwait.MatchOptions = HDMQ_GETOPT_MATCH_NONE;
        getwait.clientbuflen = 4194304;
        pQMsg = NULL;
        
        ret = HDQueueI_getMsg(pForward->pQueueI,&getoption,&getwait,&pQMsg);
        if(ret == 0)
            HDQMsg_free(pQMsg);
    }
    else
    {
        HDQMsg_free(pQMsg);
    }
    

    pthread_mutex_unlock(&pForward->mutex);

}

int DB_add_forwardDst(char *forwardQueue,HDForwardDst *pDst)
{
    int ret;
    int result;
    char sql[1024];
    
    ret = EEFAIL;
    
    sprintf(sql,"INSERT INTO \
SYS_FORWARD_INFO (FORWARD_QUEUE,DSTHOST,DSTQUEUE,WEIGHT) \
VALUES('%s','%s','%s',%d)",
        forwardQueue,
        pDst->host,
        pDst->queue,
        pDst->weight);
    
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result == SQLITE_OK)
    {
        ret = 0;
    }
    
    return ret;
}

int DB_del_forwardDst(char *forwardQueue,HDForwardDst *pDst)
{
    int ret;
    int result;
    char sql[1024];
    
    
    
    ret = EEFAIL;
    sprintf(sql,"DELETE FROM SYS_FORWARD_INFO \
WHERE FORWARD_QUEUE = '%s'  AND DSTHOST = '%s' AND DSTQUEUE = '%s'",
        forwardQueue,
        pDst->host,
        pDst->queue);
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result == SQLITE_OK)
    {
        ret = 0;
    }
    
    
    return ret;
}

int DB_upd_forwardDst_weight(char *forwardQueue,HDForwardDst *pDst)
{
    int ret;
    int result;
    char sql[1024];
    
    
    
    ret = EEFAIL;
    sprintf(sql,"UPDATE SYS_FORWARD_INFO SET WEIGHT = %d \
WHERE FORWARD_QUEUE = '%s'  AND DSTHOST = '%s' AND DSTQUEUE = '%s'",
        pDst->weight,
        forwardQueue,
        pDst->host,
        pDst->queue);
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result == SQLITE_OK)
    {
        ret = 0;
    }
    
    
    return ret;
}

HDForwardDst *HDForward_find(HDForward *pForward,char *host,char *queue)
{
    HDForwardDst *pDst;
    iter_t iter;

    pDst = NULL;
    linkedlist_iterate(pForward->pDstList,&iter);
    while((pDst = (HDForwardDst *)linkedlist_next(pForward->pDstList,&iter)) != NULL)
    {
        if((stricmp(pDst->host,host) == 0) && (stricmp(pDst->queue,queue) == 0))
            break;
    }

    return pDst;
}

int HDForward_AddDst(HDForward *pForward,char *host,char *queue,int weight)
{
    int ret;
    HDForwardDst *pDst;
    HDQueueI *pQueueI;

    pQueueI = (HDQueueI *)pForward->pQueueI;

    ret = 0;
    pthread_mutex_lock(&pForward->mutex);
    pDst = HDForward_find(pForward,host,queue);
    if(pDst)
        ret = EEEXIST;
    else
    {
        pDst = HDForwardDst_new(host,queue,weight);
        linkedlist_add(pForward->pDstList,pDst);
        DB_add_forwardDst(pQueueI->Queue.name,pDst);
    }
        
    pthread_mutex_unlock(&pForward->mutex);    
    return ret;
}

int HDForward_DelDst(HDForward *pForward,char *host,char *queue)
{
    int ret;
    HDForwardDst *pDst;
    HDQueueI *pQueueI;

    pQueueI = (HDQueueI *)pForward->pQueueI;

    ret = 0;
    pthread_mutex_lock(&pForward->mutex);
    pDst = HDForward_find(pForward,host,queue);
    if(!pDst)
        ret = EEEXIST;
    else
    {
        DB_del_forwardDst(pQueueI->Queue.name,pDst);
        linkedlist_remove_data(pForward->pDstList,pDst);
        HDForwardDst_free(pDst);
    }
        
    pthread_mutex_unlock(&pForward->mutex);    
    return ret;
}

int HDForward_UpdDstWeight(HDForward *pForward,char *host,char *queue,int weight)
{
    int ret;
    HDForwardDst *pDst;
    HDQueueI *pQueueI;

    pQueueI = (HDQueueI *)pForward->pQueueI;

    ret = 0;
    pthread_mutex_lock(&pForward->mutex);
    pDst = HDForward_find(pForward,host,queue);
    if(!pDst)
        ret = EEEXIST;
    else
    {
        pDst->weight = weight;
        DB_upd_forwardDst_weight(pQueueI->Queue.name,pDst);
    }
        
    pthread_mutex_unlock(&pForward->mutex);    
    return ret;
}

int HDForward_browse(HDForward *pForward,HdmTable table)
{
    HDForwardDst *pDst;
    iter_t iter;
    int row;
    char tmp[HDMQ_CONST_SHORT];

    pDst = NULL;
    hdt_appendcol(table,"HOST",ET_STRING);
    hdt_appendcol(table,"QUEUE",ET_STRING);
    hdt_appendcol(table,"WEIGHT",ET_STRING);
    pthread_mutex_lock(&pForward->mutex);
    
    linkedlist_iterate(pForward->pDstList,&iter);
    while((pDst = (HDForwardDst *)linkedlist_next(pForward->pDstList,&iter)) != NULL)
    {
        row = hdt_insertrow(table,-1);
        hdt_setitemstring(table,row,0,pDst->host);
        hdt_setitemstring(table,row,1,pDst->queue);
        sprintf(tmp,"%d",pDst->weight);
        hdt_setitemstring(table,row,2,tmp);
    }
    pthread_mutex_unlock(&pForward->mutex); 

    return 0;
}

