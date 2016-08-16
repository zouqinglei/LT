/*
    channel.c
    write by zouql 20081204

    1.first version 20081204

*/
#include "e_global.h"
#include "queue.h"
#include "e_log.h"
#include "channel.h"

extern char localhostip[40];
extern sqlite3 *db;   /* sqltie3 db interface */
extern HdmHandle serverHandle;
HDChannelPool ChannelPool;

HDChannel *HDChannel_new(char *host,unsigned short port,int enable,char *remark)
{
    HDChannel *pChannel;
    pChannel = E_NEW(HDChannel);

    if(pChannel)
    {
        strcpy(pChannel->host,host);
        pChannel->port = port;
        pChannel->enable = enable;
        strcpy(pChannel->remark,remark?remark:"");
    }

    return pChannel;
}

void HDChannel_free(HDChannel * pChannel)
{
     if(pChannel)
     {
         if(pChannel->handle)
             hd_term(pChannel->handle);
         FREE(pChannel);
     }
}

int HDChannelPool_init()
{
    ChannelPool.pList = linkedlist_new(0);
    pthread_mutex_init(&ChannelPool.mutex,NULL);

    /* load from db */
    HDChannelPool_BuildFromDB();
    
    ChannelPool.pChannelThread = eThread_new(10,handle_channel,NULL);
    return 0;
}

int HDChannelPool_destroy()
{
    eThread_free(ChannelPool.pChannelThread);
    linkedlist_del(ChannelPool.pList,(void *)HDChannel_free);
    pthread_mutex_destroy(&ChannelPool.mutex);

    return 0;
}

int HDChannelPool_BuildFromDB()
{
	int ret;
	int result;
	char sql[1024];
    sqlite3_stmt *stmt;
    HDChannel *pChannel;
    char *host;
    int port;
    int enable;
    char *remark;

	sprintf(sql,"SELECT DSTHOST,DSTPORT,ENABLE,REMARK FROM SYS_CHANNEL_INFO ORDER BY DSTHOST ");
						  
    result = sqlite3_prepare(db,sql,-1,&stmt,NULL);
	if(result == SQLITE_OK)
	{
		ret = 0;
		result = sqlite3_step(stmt);
		while(result == SQLITE_ROW)
		{
            host = (char *)sqlite3_column_text(stmt,0);
            port = sqlite3_column_int(stmt,1);
            enable = sqlite3_column_int(stmt,2);
            remark = (char *)sqlite3_column_text(stmt,3);
            pChannel = HDChannel_new(host,(unsigned short)port,enable,remark);

            pChannel->handle = hd_init(pChannel->host,pChannel->port,0);

	        linkedlist_add(ChannelPool.pList,pChannel);
			
			result = sqlite3_step(stmt);
		}
	}
	
	sqlite3_finalize(stmt);

	return 0;
}

int HDChannelPool_add(char *host,unsigned short port,char *remark)
{
    int ret;
    HDChannel *pChannel;
    
    ret = 0;
    pthread_mutex_lock(&ChannelPool.mutex);
    pChannel = HDChannelPool_find(host);
    if(pChannel)
        ret = EEEXIST;
    else
    {
        pChannel = HDChannel_new(host,port,1,remark);
        linkedlist_add(ChannelPool.pList,pChannel);
        DB_create_channel(pChannel);
    }
    
    pthread_mutex_unlock(&ChannelPool.mutex);
    return 0;
}

int HDChannelPool_del(char *host)
{
    int ret;
    HDChannel *pChannel;
    
    ret = 0;
    pthread_mutex_lock(&ChannelPool.mutex);
    pChannel = HDChannelPool_find(host);
    if(!pChannel)
        ret = EEEXIST;
    else
    {
        linkedlist_remove_data(ChannelPool.pList,pChannel);
        DB_delete_channel(host);
        HDChannel_free(pChannel);
    }
    
    pthread_mutex_unlock(&ChannelPool.mutex);
    return 0;
}

int HDChannelPool_browse(HdmTable table)
{
    HDChannel *pChannel;
    iter_t iter;
    int row;
    char tmp[HDMQ_CONST_SHORT];

    pChannel = NULL;
    hdt_appendcol(table,"HOST",ET_STRING);
    hdt_appendcol(table,"PORT",ET_STRING);
    hdt_appendcol(table,"LINKED",ET_STRING);
    hdt_appendcol(table,"REMARK",ET_STRING);

    pthread_mutex_lock(&ChannelPool.mutex);
    linkedlist_iterate(ChannelPool.pList,&iter);
    while((pChannel = (HDChannel *)linkedlist_next(ChannelPool.pList,&iter)) != NULL)
    {
        row = hdt_insertrow(table,-1);
        hdt_setitemstring(table,row,0,pChannel->host);
        sprintf(tmp,"%d",pChannel->port);
        hdt_setitemstring(table,row,1,tmp);
        hdt_setitemstring(table,row,2,pChannel->handle?"Y":"N");
        hdt_setitemstring(table,row,3,pChannel->remark);
    }
    pthread_mutex_unlock(&ChannelPool.mutex);

    return 0;
}

HDChannel * HDChannelPool_find(char *host)
{
    HDChannel *pChannel;
    iter_t iter;

    pChannel = NULL;
    linkedlist_iterate(ChannelPool.pList,&iter);
    while((pChannel = (HDChannel *)linkedlist_next(ChannelPool.pList,&iter)) != NULL)
    {
        if(stricmp(pChannel->host,host) == 0)
        {
            break;
        }
    }
    
    return pChannel;  
}


pthread_handler_decl(handle_channel,arg)
{
    HDChannel *pChannel;
    iter_t iter;

    pthread_mutex_lock(&ChannelPool.mutex);
    linkedlist_iterate(ChannelPool.pList,&iter);
    while((pChannel = (HDChannel *)linkedlist_next(ChannelPool.pList,&iter)) != NULL)
    {
        if(pChannel->handle == NULL)
        {
            pChannel->handle = hd_init(pChannel->host,pChannel->port,0);
        }
    }
    pthread_mutex_unlock(&ChannelPool.mutex);
}

int DB_create_channel(HDChannel *pChannel)
{
   	int result;
    char *errmsg;
	
	char sql[1024];
    int ret;
	
	errmsg = NULL;
    ret = EEFAIL;
    
    sprintf(sql,"INSERT INTO SYS_CHANNEL_INFO VALUES('%s',%d,%d,'%s')",
        pChannel->host,
        pChannel->port,
        1,
        "");
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result == SQLITE_OK)
    {
        ret = 0;
    }

	return ret;
}

int DB_delete_channel(char *host)
{
   	int result;
    char *errmsg;
	
	char sql[1024];
    int ret;
	
	errmsg = NULL;
    ret = EEFAIL;
    
    sprintf(sql,"DELETE FROM SYS_CHANNEL_INFO WHERE DSTHOST = '%s'",host);
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result == SQLITE_OK)
    {
        ret = 0;
    }

	return ret;
}

int HDChannelPool_checkLink(char *host)
{
    int ret;
    HDChannel *pChannel;

    ret = 0;
    if((strcmp(host,"127.0.0.1") == 0) ||
        strcmp(host,localhostip) == 0)
    {
        /*local put */
        return ret;
    }

    pthread_mutex_lock(&ChannelPool.mutex);
    pChannel =  HDChannelPool_find(host);
    if(!pChannel)
        ret = EENOENT;
    else
    {
        if(pChannel->handle == NULL)
            ret = EECONNECT;
    }
    pthread_mutex_unlock(&ChannelPool.mutex);

    return ret;
}

int HDChannelPool_putmsg(char *host,char *queue,HDQMsg *pQMsg)
{
    int ret;
    HDChannel *pChannel;
    HDQMsgPutOption putoption;
   
    ret = 0;

    if((strcmp(host,"127.0.0.1") == 0) ||
        strcmp(host,localhostip) == 0)
    {
        /*local put */
        putoption.Options = HDMQ_PUTOPT_NONE;
        ret = queue_put(queue,&putoption,pQMsg,NULL,NULL);
        return ret;
    }
    pthread_mutex_lock(&ChannelPool.mutex);
    pChannel =  HDChannelPool_find(host);
    if(!pChannel)
        ret = EENOENT;
    else
    {
        if(pChannel->handle == NULL)
            ret = EECONNECT;
        else
        {
            ret = HDChannel_put(pChannel,queue,pQMsg);
        }
    }
    pthread_mutex_unlock(&ChannelPool.mutex);


    return ret;
}

int HDChannel_put(HDChannel *pChannel,char *queue,HDQMsg *pQMsg)
{
    
    int ret;
    int errcod;
    HdmBuf parambuf;
    HdmBuf retbuf;
    
    errcod = 0;
    parambuf = hd_alloc();
    hd_begin_put(parambuf);
    /* queue */
    hd_putstring(parambuf,queue);
    /* option */
    hd_putint(parambuf,HDMQ_PUTOPT_NONE);
    
    /* msg header */
    hd_putstring(parambuf,pQMsg->hdr.msgId);
    hd_putstring(parambuf,pQMsg->hdr.correlId);
    hd_putint(parambuf,pQMsg->hdr.type);
    hd_putint(parambuf,pQMsg->hdr.format);
    hd_putint(parambuf,pQMsg->hdr.expiry);
    hd_putint(parambuf,pQMsg->hdr.priority);
    hd_putstring(parambuf,pQMsg->hdr.replyToQ);
    hd_putstring(parambuf,pQMsg->hdr.replyToQMgr);
    hd_putstring(parambuf,pQMsg->hdr.application);
    hd_putint(parambuf,pQMsg->hdr.createtime);
    hd_putint(parambuf,pQMsg->hdr.userTag);
    hd_putstring(parambuf,pQMsg->hdr.userRemark);

    /* msg buffer */
    hd_putblob(parambuf,pQMsg->blob,pQMsg->size);

    hd_end_put(parambuf);

    retbuf = hd_alloc();

    ret = hd_call(pChannel->handle,"queue_put",parambuf,&errcod,retbuf,0);

    if(ret == EECONNECT)
    {
        hd_term(pChannel->handle);
        pChannel->handle = NULL;
    }

    if(ret == 0)
    {
        ret = errcod;
    }

    hd_free(parambuf);
    hd_free(retbuf);

    return ret;
}

