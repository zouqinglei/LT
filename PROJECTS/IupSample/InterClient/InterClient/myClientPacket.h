/*
    myServerPacket.h
    write by zouql 20151028
    zouqinglei@163.com
*/

#ifndef INTERCLIENT_MY_CLIENT_PACKET_H
#define INTERCLIENT_MY_CLIENT_PACKET_H

#include "lt_c_foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stRequestData
{
    char * name;
    char * message;
}RequestData;

typedef struct stReplyData
{
    char * name;
    char * message;
}ReplyData;

typedef struct stParseStruct
{
    int state;

    char strContentSize[4];
    int contentSize;
    char * content;

    int index;
}ParseStruct;

#define PARSE_HEAD 0
#define PARSE_CONTENT 1


int  myClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                void * parseStruct);

void * myClientParseStructAllocFunc();

void myClientParseStructFreeFunc(void * data);

int  myClientSerialFunc(unsigned int requestID, const void * requestData, char ** buf, int * buflen);

void myClientRequestDataFreeFunc(void * data);

void myClientReplyDataFreeFunc(void * data);




#ifdef __cplusplus
}
#endif

#endif /*INTERCLIENT_MY_CLIENT_PACKET_H*/