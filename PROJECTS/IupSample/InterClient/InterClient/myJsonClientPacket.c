#include "stdio.h"
#include "stdlib.h"
#include "myJsonClientPacket.h"
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
    char  * pReplyData;

    if(pParseStruct->content == NULL)
    {
        pReplyData = STR_NEW(packet);
    }
    else
    {
        oldsize = strlen(pParseStruct->content);
        size = strlen(packet);
        pReplyData = (char *)malloc(oldsize + size + 1);
        strcpy(pReplyData, pParseStruct->content);
        strcat(pReplyData, packet);
        
        free(pParseStruct->content);
        pParseStruct->content = NULL;
    }

    return pReplyData;
}
/*
 reutrn 0 or LT_ERR_PACKET
*/
int  myJsonClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                void * parseStruct)
{
    char * pRecv;
    char * pPacket;
    char  * pReplyData;
    int i;
    int size;

 
    pRecv = (char *)recvBuff;
    pPacket = (char *)recvBuff;
    
    for(i = 0; i < recvSize; i++)
    {
        if(pRecv[i] == '\0')
        {
            pReplyData = CombineParseStruct(parseStruct, (const char * )pPacket);
            LT_Client_Append(pClient, 0, pReplyData);
            
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
        SaveToParseStruct(parseStruct, pPacket, size);
    }

    return 0;
}



void * myJsonClientParseStructAllocFunc()
{
    JsonParseStruct * pParseStruct;
    pParseStruct = (JsonParseStruct *)malloc(sizeof(JsonParseStruct));
    BZERO(pParseStruct, sizeof(JsonParseStruct));

    return pParseStruct;
}



void myJsonClientParseStructFreeFunc(void * data)
{
    JsonParseStruct * pParseStruct;
    pParseStruct = (JsonParseStruct *)data;

    if(pParseStruct->content)
        free(pParseStruct->content);

    free(pParseStruct);
}



int  myJsonClientSerialFunc(unsigned int requestID, const void * requestData, char ** buf, int * buflen)
{
    if(requestData == NULL)
        return -1;

    *buflen = strlen((const char *)requestData) + 1;
    *buf = malloc(*buflen);
    BCOPY(requestData, *buf, *buflen);

    return 0;
}



void myJsonClientRequestDataFreeFunc(void * data)
{
    free(data);
}



void myJsonClientReplyDataFreeFunc(void * data)
{
    free(data);
}

