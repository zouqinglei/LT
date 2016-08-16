#include "stdio.h"
#include "stdlib.h"
#include "myServerPacket.h"
#include "myDialog.h"

int  myServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode)
{
    ParseStruct * pParseStruct;
    char * pRecv;
    int parseSize;
    int restSize;
    int count;
    RequestData * pRequestData;
    int len;
    int tmp;

    pParseStruct = (ParseStruct * )LT_Server_GetParseStruct(pClientNode);

    pRecv = (char *)recvBuff;
    parseSize = 0;
    restSize = recvSize; 

    while(restSize > 0)
    {
        if(pParseStruct->state == PARSE_HEAD)
        {
            if((pParseStruct->index + restSize) < 4)
            {
                BCOPY(pRecv, &pParseStruct->strContentSize[pParseStruct->index], restSize);
                pParseStruct->index += restSize;
                return LT_ERR_BUF;
            }
            else
            {
                count = 4 - pParseStruct->index;
                
                BCOPY(pRecv, &pParseStruct->strContentSize[pParseStruct->index], count);
                pRecv += count;
                restSize -= count;
                

                pParseStruct->state = PARSE_CONTENT;
                pParseStruct->index = 0;

                //parse head
                BCOPY(pParseStruct->strContentSize, &tmp, 4);
                pParseStruct->contentSize = ntohl(tmp);
                
            }
        }
        else if(pParseStruct->state == PARSE_CONTENT)
        {
            if(pParseStruct->content == NULL)
            {
                pParseStruct->content = (char *)malloc(pParseStruct->contentSize);
                pParseStruct->index = 0;
            }

            if((pParseStruct->index + restSize) < pParseStruct->contentSize)
            {
                BCOPY(pRecv,&pParseStruct->content[pParseStruct->index],restSize);
                pParseStruct->index += restSize;
                return LT_ERR_BUF;
            }
            else
            {
                count = pParseStruct->contentSize - pParseStruct->index;
                BCOPY(pRecv, &pParseStruct->content[pParseStruct->index], count);
                pRecv += count;
                restSize -= count;

                //get a requestdata
                pRequestData = (RequestData *)malloc(sizeof(RequestData));
                pRequestData->name = STR_NEW(pParseStruct->content);
                len = strlen(pParseStruct->content);
                pRequestData->message = STR_NEW(pParseStruct->content + len + 1);

                LT_Server_Append(pServer, pClientNode, 0, pRequestData);

                //reset the parseStruct for next loop
                pParseStruct->state = PARSE_HEAD;
                if(pParseStruct->content != NULL)
                {
                    free(pParseStruct->content);
                    pParseStruct->content = NULL;
                }
                pParseStruct->index = 0;
            }
        }
    }

    return 0;
}



void * myServerParseStructAllocFunc()
{
    ParseStruct * pParseStruct;
    pParseStruct = (ParseStruct *)malloc(sizeof(ParseStruct));
    BZERO(pParseStruct, sizeof(ParseStruct));

    return pParseStruct;
}



void myServerParseStructFreeFunc(void * data)
{
    ParseStruct * pParseStruct;
    pParseStruct = (ParseStruct *)data;

    if(pParseStruct->content)
        free(pParseStruct->content);

    free(pParseStruct);
}



int  myServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen)
{
    int tmp;
    int pos;
    int len;

    char * contentbuf;
    int contentlen;
    ReplyData * pReplyData;

    if(pInfo->replyData == NULL)
        return -1;

    contentbuf = NULL;

    pReplyData = (ReplyData *)pInfo->replyData;

    
    contentlen = strlen(pReplyData->name) + 1 + strlen(pReplyData->message) + 1;
    *buflen = contentlen + 4;
    *buf = (char *)malloc(*buflen);

    pos = 0;
    /* contentSize */
    tmp = htonl(contentlen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;
   
    len = strlen(pReplyData->name) + 1;
    BCOPY(pReplyData->name, *buf + pos, len);
    pos += len;

    len = strlen(pReplyData->message) + 1;
    BCOPY(pReplyData->message, *buf + pos, len);

    return 0;
}



void myServerRequestDataFreeFunc(void * data)
{
    RequestData * pRequestData;

    pRequestData = (RequestData *)data;
    if(pRequestData->name)
        free(pRequestData->name);
    if(pRequestData->message)
        free(pRequestData->message);

    free(pRequestData);
}



void myServerReplyDataFreeFunc(void * data)
{
    RequestData * pRequestData;

    pRequestData = (RequestData *)data;
    if(pRequestData->name)
        free(pRequestData->name);
    if(pRequestData->message)
        free(pRequestData->message);

    free(pRequestData);
}



void myServerProcessFunc(LT_RequestInfo * pInfo)
{
    RequestData * pRequestData;
    ReplyData * pReplyData;

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

    
    pRequestData = (RequestData *)pInfo->requestData;
    printf("[%s] send: %s \n",pRequestData->name, pRequestData->message);

    pReplyData = (ReplyData *)malloc(sizeof(ReplyData));
    pReplyData->name = STR_NEW(pRequestData->name);
    pReplyData->message = STR_NEW(pRequestData->message);

    LT_Server_Send(pInfo->pServer, 0, 0, pReplyData);

    pInfo->dontReply = 1;
}

