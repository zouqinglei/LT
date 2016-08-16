/*
    memqueue.h
    manager queue and message store in memory

    write by zouql 20081103
    1. first version 20081103
*/

#ifndef E_MEM_QUEUE_17_H_
#define E_MEM_QUEUE_17_H_

#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

int Mem_addMsg();
int Mem_getMsg(HDQueueI *pQueueI,HDQMsgGetOption *pGetOption,HDGetWait *pGetWait,HDQMsg **ppQMsg);
int Mem_queuesize();
int Mem_browsemsg(HDQueueI *pQueueI,int start,int count,HdmTable table);
int Mem_clear(HDQueueI *pQueueI);
int Mem_delete_ttl(HDQueueI *pQueueI);

#ifdef __cplusplus
}
#endif


#endif /*E_MEM_QUEUE_17_H_*/

