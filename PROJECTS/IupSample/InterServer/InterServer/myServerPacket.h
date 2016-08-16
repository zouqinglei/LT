/*
    myServerPacket.h
    write by zouql 20151028
    zouqinglei@163.com
*/

#ifndef INTERSERVER_MY_SERVER_PACKET_H
#define INTERSERVER_MY_SERVER_PACKET_H

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


int  myServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode);

void * myServerParseStructAllocFunc();

void myServerParseStructFreeFunc(void * data);

int  myServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen);

void myServerRequestDataFreeFunc(void * data);

void myServerReplyDataFreeFunc(void * data);

void myServerProcessFunc(LT_RequestInfo * pInfo);



#ifdef __cplusplus
}
#endif

#endif /*INTERSERVER_MY_SERVER_PACKET_H*/