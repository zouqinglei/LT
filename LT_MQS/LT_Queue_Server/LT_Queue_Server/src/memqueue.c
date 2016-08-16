/*
   memequeue.c

   mem queue module 
*/
#include "e_global.h"
#include "e_util.h"

#include "memqueue.h"


int Mem_addMsg()
{
    return 0;
}

int Mem_getMsg(HDQueueI *pQueueI,HDQMsgGetOption *pGetOption,HDGetWait *pGetWait,HDQMsg **ppQMsg)
{
    int ret;
    HDQMsg *pQMsg;
    HDQMsg *pCopyQMsg;
    iter_t iter;

    ret = 0;
	pQMsg = NULL;
    
    linkedlist_iterate(pQueueI->pMsgList,&iter);
    while((pQMsg = (HDQMsg *)linkedlist_next(pQueueI->pMsgList,&iter)) != NULL)
	{
        if(pGetWait->MatchOptions == HDMQ_GETOPT_MATCH_NONE)
            break;
        if(pGetWait->MatchOptions == HDMQ_GETOPT_MATCH_MSG_ID)
        {
            if(stricmp(pQMsg->hdr.msgId,pGetWait->msgId) == 0)
                break;
        }
        if(pGetWait->MatchOptions == HDMQ_GETOPT_MATCH_CORREL_ID)
        {
            if(stricmp(pQMsg->hdr.correlId,pGetWait->correlId) == 0)
                break;
        }
        if(pGetWait->MatchOptions == HDMQ_GETOPT_MATCH_USER_TAG)
        {
            if(pQMsg->hdr.userTag == pGetWait->userTag)
                break;
        }
    }
    
    if(pQMsg == NULL)
        return EENODATA;
    /* compare buf realsize and client supply's size */
    if(pQMsg->size > pGetWait->clientbuflen)
    {
        if(!(pGetOption->Options & HDMQ_GETOPT_ACCEPT_TRUNCATED_MSG))
            return EELARGEDATA;
    }
    
    if(pGetOption->Options & HDMQ_GETOPT_BROWSE)
    {
        pCopyQMsg = HDQMsg_new();
        BCOPY(&pQMsg->hdr,&pCopyQMsg->hdr,sizeof(HDQMsgHdr));
        pCopyQMsg->blob = NEW(pQMsg->size);
        BCOPY(pQMsg->blob,pCopyQMsg->blob,pQMsg->size);
        pCopyQMsg->size = pQMsg->size;
        *ppQMsg = pCopyQMsg;
    }
    else
    {
        linkedlist_remove_data(pQueueI->pMsgList,pQMsg);
        *ppQMsg = pQMsg;
    }

	return ret;
}

int Mem_browsemsg(HDQueueI *pQueueI,int start,int count,HdmTable table)
{
	int ret;
	HDQMsg *pQMsg;
    iter_t iter;
    int row;
    int col;
    char tmpbuf[HDM_CONST_LONG];
    int i;
    int n;
	ret = 0;
    i = 0;
    n = 0;

    if(start < 0)
        start = 0;
    if(count < 0)
        count = linkedlist_size(pQueueI->pMsgList);
    linkedlist_iterate(pQueueI->pMsgList,&iter);
    while((pQMsg = (HDQMsg *)linkedlist_next(pQueueI->pMsgList,&iter)) != NULL)
    {
        if(i < start)
        {
            i++;
            continue;
        }

        if(n == count)
        {
            break;
        }
        n++;

        col = 0;
        row = hdt_insertrow(table,-1);
        hdt_setitemstring(table,row,col++,pQMsg->hdr.msgId);
        hdt_setitemstring(table,row,col++,pQMsg->hdr.correlId);

        sprintf(tmpbuf,"%d",pQMsg->hdr.type);
        hdt_setitemstring(table,row,col++,tmpbuf);

        sprintf(tmpbuf,"%d",pQMsg->hdr.format);
        hdt_setitemstring(table,row,col++,tmpbuf);

        sprintf(tmpbuf,"%d",pQMsg->hdr.expiry);
        hdt_setitemstring(table,row,col++,tmpbuf);

        sprintf(tmpbuf,"%d",pQMsg->hdr.priority);
        hdt_setitemstring(table,row,col++,tmpbuf);

        hdt_setitemstring(table,row,col++,pQMsg->hdr.replyToQ);
        hdt_setitemstring(table,row,col++,pQMsg->hdr.replyToQMgr);
        hdt_setitemstring(table,row,col++,pQMsg->hdr.application);

        sprintf(tmpbuf,"%s",e_strftime((time_t *)&pQMsg->hdr.createtime));
        hdt_setitemstring(table,row,col++,tmpbuf);

        sprintf(tmpbuf,"%d",pQMsg->hdr.userTag);
        hdt_setitemstring(table,row,col++,tmpbuf);

        hdt_setitemstring(table,row,col++,pQMsg->hdr.userRemark);

        sprintf(tmpbuf,"%d",pQMsg->size);
        hdt_setitemstring(table,row,col++,tmpbuf);
      
    }
    
	return ret;
}

int Mem_queuesize()
{
    return 0;
}

int Mem_clear(HDQueueI *pQueueI)
{
    linkedlist_clear(pQueueI->pMsgList,(void *)HDQMsg_free);
    return 0;
}

int Mem_delete_ttl(HDQueueI *pQueueI)
{
    int ret;
    HDQMsg *pQMsg;
    int i;
    int count;
    int size;
    time_t now;

    now = time(NULL);

    ret = 0;    
    count = 0;

    size = linkedlist_size(pQueueI->pMsgList);

    for(i = size - 1; i >= 0; i--)
    {
        pQMsg = (HDQMsg *)linkedlist_get(pQueueI->pMsgList,i);
        if(pQMsg->hdr.expiry <= 0)
            continue;
        if((now - pQMsg->hdr.createtime) > pQMsg->hdr.expiry)
        {
            linkedlist_remove(pQueueI->pMsgList,i);
            HDQMsg_free(pQMsg);
            count++;
        }
    }
    
    ret = count;
    return ret;
}

