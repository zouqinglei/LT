/*
    myJsonServerPacket.h
    write by zouql 20151102
    zouqinglei@163.com
*/

#ifndef INTERSERVER_MY_CLIENT_PACKET_JSON_H
#define INTERSERVER_MY_CLIENT_PACKET_JSON_H

#include "lt_c_foundation.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct stJsonParseStruct
{
    char * content;

}JsonParseStruct;

int  myJsonClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                void * parseStruct);

void * myJsonClientParseStructAllocFunc();

void myJsonClientParseStructFreeFunc(void * data);

int  myJsonClientSerialFunc(unsigned int requestID, const void * requestData, char ** buf, int * buflen);

void myJsonClientRequestDataFreeFunc(void * data);

void myJsonClientReplyDataFreeFunc(void * data);



#ifdef __cplusplus
}
#endif

#endif /*INTERSERVER_MY_CLIENT_PACKET_JSON_H*/