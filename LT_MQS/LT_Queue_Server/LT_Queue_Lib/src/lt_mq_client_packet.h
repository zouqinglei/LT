/* 
 * lt_mq_client_packet.h
 * write by zouql 20140724
 * zouqinglei@163.com
 * packet 

 * modifty by zouql 20150817 for new client/server socket foundation
 */

#include "lt_c_foundation.h"


#ifndef LT_MQ_CLIENT_PACKET_H
#define LT_MQ_CLIENT_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stLTMQ_PacketHead
{
    int magic; /* LTMQ */   
    int protocol;
    int flag;      /* reserved,active,encrypt,compress */
    unsigned int requestID;
    int contentlen; 
    int oricontentlen;

}LTMQ_PacketHead;


#define LTMQ_PACKET_HEAD_SIZE 24
#define LTMQ_PACKET_CONTENT_SIZE_MAX (4*1024*1024)

#define LTMQ_PACKET_FLAG_COMPRESS 0x00000001


typedef struct stLTMQ_ParseStruct
{
    int state;

    LTMQ_PacketHead head;

    char headbuf[LTMQ_PACKET_HEAD_SIZE];
    char * contentbuf;

    int index;
}LTMQ_ParseStruct;


#define LTMQ_PARSE_HEAD 0
#define LTMQ_PARSE_CONTENT 1



int  ltmqClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                         void * parseStruct);

void * ltmqClientParseStructAllocFunc();

void ltmqClientParseStructFreeFunc(void * data);

int  ltmqClientSerialFunc(const unsigned int requestID, const void * requestData, char ** buf, int * buflen);

void ltmqClientRequestDataFreeFunc(void * data);

void ltmqClientReplyDataFreeFunc(void * data);


#ifdef __cplusplus
}
#endif

#endif /*LT_MQ_CLIENT_PACKET_H*/
