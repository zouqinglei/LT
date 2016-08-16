/*
  channel.h
  link channel to other HDMQ server.
  manager link,keep and so on

  write by zouql 20081204
*/

#ifndef E_CHANNEL_17_H_
#define E_CHANNEL_17_H_

#include "e_global.h"
#include "e_pthread.h"
#include "mba/linkedlist.h"
#include "e_util.h"
#include "e_thread.h"

typedef struct 
{
    char host[HDMQ_CONST_SHORT];
    unsigned short port;

    int enable;
    char remark[HDMQ_CONST_LONG];
    HdmHandle handle;

    int ref;
}HDChannel;

typedef struct 
{
    struct linkedlist *pList;
    pthread_mutex_t mutex;
    EThread *pChannelThread;
}HDChannelPool;

HDChannel *HDChannel_new(char *host,unsigned short port,int enable,char *remark);
void HDChannel_free(HDChannel * pChannel);
int HDChannel_put(HDChannel *pChannel,char *queue,HDQMsg *pQMsg);

int HDChannelPool_init();
int HDChannelPool_destroy();
int HDChannelPool_BuildFromDB();
HDChannel * HDChannelPool_find(char *host);

int DB_create_channel(HDChannel *pChannel);
int DB_delete_channel(char *host);

pthread_handler_decl(handle_channel,arg);

/*intface*/
int HDChannelPool_add(char *host,unsigned short port,char *remark);
int HDChannelPool_del(char *host);
int HDChannelPool_browse(HdmTable table);
int HDChannelPool_checkLink(char *host);
int HDChannelPool_putmsg(char *host,char *queue,HDQMsg *pQMsg);

#endif /* E_CHANNEL_17_H_ */

