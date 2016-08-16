/* 
 * lt_mq_packet.h
 * write by zouql 20140724
 * zouqinglei@163.com
 * packet 
 */

#include "lt_c_foundation.h"


#ifndef LT_MQ_PACKET_H
#define LT_MQ_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stLTMQPacketHeader
{
    int magic; /* LTMQ */   
    int protocol;
    int reqid;
    int flag;      /* reserved,active,encrypt,compress */
    int headlen; 
    int oriheadlen;
    int bloblen; 
    int oribloblen;
    int reserved0;
    int reserved1;
    int reserved2;
    int reserved3;
}LTMQ_PacketLead;

#define LTMQ_PACKET_LEAD_SIZE 48
#define LTMQ_PACKET_HEAD_SIZE_MAX (4*1024*1024)
#define LTMQ_PACKET_BLOB_SIZE_MAX (4*1024*1024)


#define LTMQ_PACKET_FLAG_COMPRESS 0x00000001
#define LTMQ_PACKET_FLAG_ENCRYPT_HEAD  0x00000002
#define LTMQ_PACKET_FLAG_ENCRYPT_BLOB  0x00000004
#define LTMQ_PACKET_FLAG_ACTIVE   0x00000008


typedef struct stLTMQ_UserData
{
    struct json_object * headobj;
    int bloblen;
    void * blob;
}LTMQ_UserData;

LTMQ_UserData * ltmq_userdata_new();
void ltmq_userdata_free(void * userdata);

int ltmq_packet_parse(CIRCBUF * pbuf, unsigned int * requestID, void ** userdata);
int ltmq_packet_packet(const unsigned int requestID, const void * userdata,  char ** buf, int * buflen);
int ltmq_packet_active(char ** buf, int * buflen);


/*
request headobj struct
{
    'reqname':'PUT_MESSAGE',

    'in_param':
    {
    }
}

reply headobj struct
{
    'reqname':'PUT_MESSAGE',

    'out_param':
    {
    }
}
*/

#ifdef __cplusplus
}
#endif

#endif /*LT_MQ_PACKET_H*/
