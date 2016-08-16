#include "stdio.h"
#include "stdlib.h"
#include "myServer.h"

#include "lt_c_foundation.h"

#include "myServerPacket.h"

#include "myJsonServerPacket.h"

LT_ServerParam serverParam;

LT_Server * pServer;

#define MY_JSON_PARSE 0

void initServer()
{
    lt_initial();

    serverParam.listenPort = 9001;
    serverParam.maxClient = 10;
    serverParam.maxRequestSize = 0;
    serverParam.maxWorkThreadSize = 5;
#ifndef MY_JSON_PARSE 
    serverParam.parseFunc = myServerParseFunc;
    serverParam.parseStructAllocFunc = myServerParseStructAllocFunc;
    serverParam.parseStructFreeFunc = myServerParseStructFreeFunc;
    serverParam.serialFunc = myServerSerialFunc;
    serverParam.requestDataFreeFunc = myServerRequestDataFreeFunc;
    serverParam.replyDataFreeFunc = myServerReplyDataFreeFunc;
    serverParam.processFunc = myServerProcessFunc;
#else
    serverParam.parseFunc = myJsonServerParseFunc;
    serverParam.parseStructAllocFunc = myJsonServerParseStructAllocFunc;
    serverParam.parseStructFreeFunc = myJsonServerParseStructFreeFunc;
    serverParam.serialFunc = myJsonServerSerialFunc;
    serverParam.requestDataFreeFunc = myJsonServerRequestDataFreeFunc;
    serverParam.replyDataFreeFunc = myJsonServerReplyDataFreeFunc;
    serverParam.processFunc = myJsonServerProcessFunc;

#endif
    pServer = LT_Server_New(&serverParam);   
}

void startServer()
{
    LT_Server_Start(pServer);
}

void stopServer()
{
    LT_Server_Stop(pServer);
}

void freeServer()
{
    LT_Server_Free(pServer);
}

