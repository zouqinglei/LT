/* 
 * lt_mq_server_packet.c
 * write by zouql 20140724
 * zouqinglei@163.com
 * packet 
 * 
 * 20150817
 * modify by zouql for new client/socket model
 */

#include "lt_mq_client_packet.h"

/*
    if success, return 0;
    else return LT_ERR_PACKET
*/
static int ltmqServerParseHead(LTMQ_ParseStruct * parseSt)
{
    int pos;
    int tmp;

    pos = 0;

    BCOPY(&parseSt->headbuf[pos], (char *)&tmp, 4);
    parseSt->head.magic = ntohl(tmp);
    pos += 4;

    BCOPY(&parseSt->headbuf[pos], (char *)&tmp, 4);
    parseSt->head.protocol = ntohl(tmp);
    pos += 4;

    BCOPY(&parseSt->headbuf[pos], (char *)&tmp, 4);
    parseSt->head.flag = ntohl(tmp);
    pos += 4;

    BCOPY(&parseSt->headbuf[pos], (char *)&tmp, 4);
    parseSt->head.requestID = ntohl(tmp);
    pos += 4;

    BCOPY(&parseSt->headbuf[pos], (char *)&tmp, 4);
    parseSt->head.contentlen = ntohl(tmp);
    pos += 4;

    BCOPY(&parseSt->headbuf[pos], (char *)&tmp, 4);
    parseSt->head.oricontentlen = ntohl(tmp);
    pos += 4;


    if((parseSt->head.magic !=  0x4C544D51) ||
        (parseSt->head.protocol !=  0x00010000) ||
        (parseSt->head.contentlen > LTMQ_PACKET_CONTENT_SIZE_MAX))
    {
        return LT_ERR_PACKET;
    }

    return 0;
    
}

static void ltmqClientGetRequestData(LT_Client * pClient, LTMQ_ParseStruct * parseSt)
{
    int ret;
    int  count;
    char * oricontent;
    json_object * replyObj;

    //content if compress
    if(parseSt->head.flag & LTMQ_PACKET_FLAG_COMPRESS) 
    {
        count = parseSt->head.oricontentlen + 4096;
        oricontent = (char *)malloc(count);
        ret = uncompress(oricontent, &count, parseSt->contentbuf, parseSt->head.contentlen);

        if(ret != Z_OK)
        {
        }

        replyObj = json_tokener_parse(oricontent);
        if(is_error(replyObj))
        {
        }
        
        LT_Client_Append(pClient, parseSt->head.requestID,replyObj);
        free(parseSt->contentbuf);
        free(oricontent);

    }
    else
    {
        replyObj = json_tokener_parse(parseSt->contentbuf);
        if(is_error(replyObj))
        {
        }
        LT_Client_Append(pClient, parseSt->head.requestID, replyObj);
        free(parseSt->contentbuf);

    }

    parseSt->contentbuf = NULL;
    parseSt->index = 0;
    
}



int  ltmqClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                         void * parseStruct)
{
    LTMQ_ParseStruct * parseSt;
    char * pRecv;
    int parseSize;
    int restSize;
    int count;

    parseSt = (LTMQ_ParseStruct * )parseStruct;
    pRecv = (char *)recvBuff;
    parseSize = 0;
    restSize = recvSize; 

    while(restSize > 0)
    {
        if(parseSt->state == LTMQ_PARSE_HEAD)
        {
            if((parseSt->index + restSize) < LTMQ_PACKET_HEAD_SIZE)
            {
                BCOPY(pRecv, parseSt->headbuf, restSize);
                parseSt->index += restSize;
                return LT_ERR_BUF;
            }
            else
            {
                count = LTMQ_PACKET_HEAD_SIZE - parseSt->index;
                
                BCOPY(pRecv, &parseSt->headbuf[parseSt->index], count);
                pRecv += count;
                restSize -= count;
                

                parseSt->state = LTMQ_PARSE_CONTENT;
                parseSt->index = 0;

                //parse head
                if(ltmqServerParseHead(parseSt) < 0)
                    return LT_ERR_PACKET;
            }
        }
        else if(parseSt->state == LTMQ_PARSE_CONTENT)
        {
            if(parseSt->contentbuf == NULL)
            {
                parseSt->contentbuf = (char *)malloc(parseSt->head.contentlen);
                parseSt->index = 0;
            }

            if((parseSt->index + restSize) < parseSt->head.contentlen)
            {
                BCOPY(pRecv,&parseSt->contentbuf[parseSt->index],restSize);
                parseSt->index += restSize;
                return LT_ERR_BUF;
            }
            else
            {
                count = parseSt->head.contentlen - parseSt->index;
                BCOPY(pRecv, &parseSt->contentbuf[parseSt->index], count);
                pRecv += count;
                restSize -= count;

                //get a requestdata
                ltmqClientGetRequestData(pClient, parseSt);
                
                parseSt->state = LTMQ_PARSE_HEAD;
                if(parseSt->contentbuf != NULL)
                {
                    free(parseSt->contentbuf);
                    parseSt->contentbuf = NULL;
                }
                parseSt->index = 0;
            }
        }
    }

    return 0;
}



void * ltmqClientParseStructAllocFunc()
{
    LTMQ_ParseStruct * parseStruct;

    parseStruct = (LTMQ_ParseStruct *)malloc(sizeof(LTMQ_ParseStruct));

    parseStruct->state = LTMQ_PARSE_HEAD;
    parseStruct->contentbuf = NULL;
    parseStruct->index = 0;

    return parseStruct;
}

void ltmqClientParseStructFreeFunc(void * data)
{
    LTMQ_ParseStruct * parseStruct;

    parseStruct = (LTMQ_ParseStruct *)data;
    if(parseStruct)
    {
        if(parseStruct->contentbuf != NULL)
        {
            free(parseStruct->contentbuf);
        }

        free(parseStruct);
    }
}


int  ltmqClientSerialFunc(const unsigned int requestID, const void * requestData, char ** buf, int * buflen)
{
    int ret;
    int tmp;

    LTMQ_PacketHead head;
    int pos;

    char * contentbuf;
    int contentlen;

    const char * requestbuf;

    if(requestData == NULL)
        return -1;

    requestbuf = jo_to_json_string((json_object *)requestData);

    contentbuf = NULL;

    BZERO(&head, sizeof(LTMQ_PacketHead));

    head.requestID = requestID;
    head.oricontentlen = strlen(requestbuf) + 1;
    
    //compress
    if(head.oricontentlen >= 1024)
    {
        head.flag |= LTMQ_PACKET_FLAG_COMPRESS;
        contentlen = head.oricontentlen + 4096;
        contentbuf = (char *)malloc(contentlen);
        ret = compress(contentbuf, &contentlen, (unsigned char  *)requestbuf, head.oricontentlen);
        head.contentlen = contentlen;
    }
    else
    {
        head.contentlen = head.oricontentlen;
    }

         
    *buflen = LTMQ_PACKET_HEAD_SIZE + head.contentlen;

    *buf = (char *)malloc(*buflen);

    pos = 0;
    /* magic */
    tmp = htonl(0x4C544D51);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;
    /* protocol */
    tmp = htonl(0x00010000);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* flag */
    tmp = htonl(head.flag);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reqid */
    tmp = htonl(head.requestID);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* headlen */
    tmp = htonl(head.contentlen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* oriheadlen */
    tmp = htonl(head.oricontentlen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;



    if(head.flag & LTMQ_PACKET_FLAG_COMPRESS)
    {
        BCOPY(contentbuf, *buf + LTMQ_PACKET_HEAD_SIZE, head.contentlen);
        free(contentbuf);
    }
    else
    {
        BCOPY(requestbuf, *buf + LTMQ_PACKET_HEAD_SIZE, head.contentlen);
    }

    return 0;
}

/*
 * server_free_data_func()
 *
 * 
 */

void ltmqClientRequestDataFreeFunc(void * data)
{
    jo_put((json_object *)data);
}

void ltmqClientReplyDataFreeFunc(void * data)
{
    jo_put((json_object *)data);
}
