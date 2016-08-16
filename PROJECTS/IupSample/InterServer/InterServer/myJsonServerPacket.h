/*
    myJsonServerPacket.h
    write by zouql 20151028
    zouqinglei@163.com
*/

#ifndef INTERSERVER_MY_SERVER_PACKET_JSON_H
#define INTERSERVER_MY_SERVER_PACKET_JSON_H

#include "lt_c_foundation.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct stJsonParseStruct
{
    char * content;

}JsonParseStruct;

int  myJsonServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode);

void * myJsonServerParseStructAllocFunc();

void myJsonServerParseStructFreeFunc(void * data);

int  myJsonServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen);

void myJsonServerRequestDataFreeFunc(void * data);

void myJsonServerReplyDataFreeFunc(void * data);

void myJsonServerProcessFunc(LT_RequestInfo * pInfo);



#ifdef __cplusplus
}
#endif

#endif /*INTERSERVER_MY_SERVER_PACKET_JSON_H*/