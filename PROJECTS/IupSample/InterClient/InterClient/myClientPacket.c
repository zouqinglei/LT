#include "stdio.h"
#include "stdlib.h"
#include "myClientPacket.h"

int  myClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                void * parseStruct)
{
    ParseStruct * pParseStruct;
    char * pRecv;
    int parseSize;
    int restSize;
    int count;
    ReplyData * pReplyData;
    int len;
    int tmp;

    pParseStruct = (ParseStruct * )parseStruct;

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
                pReplyData = (ReplyData *)malloc(sizeof(ReplyData));
                pReplyData->name = STR_NEW(pParseStruct->content);
                len = strlen(pParseStruct->content);
                pReplyData->message = STR_NEW(pParseStruct->content + len + 1);

                LT_Client_Append(pClient, 0, pReplyData);

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



void * myClientParseStructAllocFunc()
{
    ParseStruct * pParseStruct;
    pParseStruct = (ParseStruct *)malloc(sizeof(ParseStruct));
    BZERO(pParseStruct, sizeof(ParseStruct));

    return pParseStruct;
}



void myClientParseStructFreeFunc(void * data)
{
    ParseStruct * pParseStruct;
    pParseStruct = (ParseStruct *)data;

    if(pParseStruct->content)
        free(pParseStruct->content);

    free(pParseStruct);
}



int  myClientSerialFunc(unsigned int requestID, const void * requestData, char ** buf, int * buflen)
{
    int tmp;
    int pos;
    int len;

    char * contentbuf;
    int contentlen;
    RequestData * pRequestData;

    if(requestData == NULL)
        return -1;

    contentbuf = NULL;

    pRequestData = (RequestData *)requestData;

    
    contentlen = strlen(pRequestData->name) + 1 + strlen(pRequestData->message) + 1;
    *buflen = contentlen + 4;
    *buf = (char *)malloc(*buflen);

    pos = 0;
    /* contentSize */
    tmp = htonl(contentlen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;
   
    len = strlen(pRequestData->name) + 1;
    BCOPY(pRequestData->name, *buf + pos, len);
    pos += len;

    len = strlen(pRequestData->message) + 1;
    BCOPY(pRequestData->message, *buf + pos, len);

    return 0;
}



void myClientRequestDataFreeFunc(void * data)
{
    RequestData * pRequestData;

    pRequestData = (RequestData *)data;
    if(pRequestData->name)
        free(pRequestData->name);
    if(pRequestData->message)
        free(pRequestData->message);

    free(pRequestData);
}



void myClientReplyDataFreeFunc(void * data)
{
    RequestData * pRequestData;

    pRequestData = (RequestData *)data;
    if(pRequestData->name)
        free(pRequestData->name);
    if(pRequestData->message)
        free(pRequestData->message);

    free(pRequestData);
}
