#include "stdio.h"
#include "stdlib.h"
#include "myJsonServerPacket.h"
#include "myDialog.h"


static void SaveToParseStruct(JsonParseStruct * pParseStruct, const char * left, int size)
{
    int oldsize;
    char * pTmp;

    if(pParseStruct->content == NULL)
    {
        pParseStruct->content = malloc(size + 1);
        BCOPY(left, pParseStruct->content, size);
        pParseStruct->content[size] = '\0';
    }
    else
    {
      
        oldsize = strlen(pParseStruct->content);
        pTmp = pParseStruct->content;
        pParseStruct->content = malloc(size + oldsize + 1);
        BCOPY(pTmp, pParseStruct->content, oldsize);
        free(pTmp);
        BCOPY(left, pParseStruct->content + oldsize, size);
        pParseStruct->content[size + oldsize] = '\0';
    }
}

static char * CombineParseStruct(JsonParseStruct * pParseStruct, const char * packet)
{
    int oldsize, size;
    char  * pRequestData;

    if(pParseStruct->content == NULL)
    {
        pRequestData = STR_NEW(packet);
    }
    else
    {
        oldsize = strlen(pParseStruct->content);
        size = strlen(packet);
        pRequestData = (char *)malloc(oldsize + size + 1);
        strcpy(pRequestData, pParseStruct->content);
        strcat(pRequestData, packet);
        
        free(pParseStruct->content);
        pParseStruct->content = NULL;
    }

    return pRequestData;
}


/*
 reutrn 0 or LT_ERR_PACKET
*/
int  myJsonServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode)
{
    JsonParseStruct * pParseStruct;
    char * pRecv;
    char * pPacket;
    char  * pRequestData;
    int i;
    int size;

    pParseStruct = (JsonParseStruct * )LT_Server_GetParseStruct(pClientNode);

    pRecv = (char *)recvBuff;
    pPacket = (char *)recvBuff;
    
    for(i = 0; i < recvSize; i++)
    {
        if(pRecv[i] == '\0')
        {
            pRequestData = CombineParseStruct(pParseStruct, (const char * )pPacket);
            LT_Server_Append(pServer, pClientNode, 0, pRequestData);
            
            if(i < recvSize - 1)
            {
               pPacket = pRecv + i + 1;
            }
            else
            {
                pPacket = NULL;
            }
        }
    }

    //left some bytes
    if(pPacket != NULL)
    {
        size = recvSize - (pPacket - pRecv);
        SaveToParseStruct(pParseStruct, pPacket, size);
    }

    return 0;
}



void * myJsonServerParseStructAllocFunc()
{
    JsonParseStruct * pParseStruct;
    pParseStruct = (JsonParseStruct *)malloc(sizeof(JsonParseStruct));
    BZERO(pParseStruct, sizeof(JsonParseStruct));

    return pParseStruct;
}



void myJsonServerParseStructFreeFunc(void * data)
{
    JsonParseStruct * pParseStruct;
    pParseStruct = (JsonParseStruct *)data;

    if(pParseStruct->content)
        free(pParseStruct->content);

    free(pParseStruct);
}



int  myJsonServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen)
{
    if(pInfo->replyData == NULL)
        return -1;

    *buflen = strlen((const char *)pInfo->replyData) + 1;
    *buf = malloc(*buflen);
    BCOPY(pInfo->replyData, *buf, *buflen);

    return 0;
}



void myJsonServerRequestDataFreeFunc(void * data)
{
    free(data);
}



void myJsonServerReplyDataFreeFunc(void * data)
{
    free(data);
}



void myJsonServerProcessFunc(LT_RequestInfo * pInfo)
{
    int len;
    void * pReplyData;

    if(pInfo->ltEvent == LT_SERVER_EVENT_CONNECT)
    {
        printf("one client connect %d\n",pInfo->clientID);
        myMatrixAppend(pInfo->clientID, inet_ntoa(pInfo->remote.sin_addr), pInfo->remote.sin_port);
        return;
    }
    else if(pInfo->ltEvent == LT_SERVER_EVENT_DISCONNECT)
    {
        printf("one client disconnect %d\n",pInfo->clientID);
        myMatrixDelete(pInfo->clientID);
        return;
    } 

    len = strlen((const char *)pInfo->requestData) + 1;
    pReplyData = malloc(len);
    BCOPY(pInfo->requestData, pReplyData, len);

    LT_Server_Send(pInfo->pServer, 0, 0, pReplyData);

    pInfo->dontReply = 1;
}

