/* 
 * lt_mq_server_packet.h
 * write by zouql 20140724
 * zouqinglei@163.com
 * packet 

 * modifty by zouql 20150817 for new client/server socket foundation
 */

#include "lt_c_foundation.h"


#ifndef LT_MQ_SERVER_PACKET_H
#define LT_MQ_SERVER_PACKET_H

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

/*
 * server_packet_parse_func()
 * return 1: active packet
 *        0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */
int  ltmqServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode);



void * ltmqServerParseStructAllocFunc();

void ltmqServerParseStructFreeFunc(void * data);

/*
 * server_packet_packet_func()
 *
 * return 0: if packet success
 * else is error
 */

int  ltmqServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen);

/*
 * server_free_data_func()
 *
 * 
 */

void ltmqServerRequestDataFreeFunc(void * data);

void ltmqServerReplyDataFreeFunc(void * data);

/*
 * server_process_func()
 * process the pInfo->requestdata and reply to the replydata
 */
void ltmqServerProcessFunc(LT_RequestInfo * pInfo);


#ifdef __cplusplus
}
#endif

#endif /*LT_MQ_SERVER_PACKET_H*/
