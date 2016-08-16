#include "stdio.h"
#include "stdlib.h"
#include "myServer.h"

#include "lt_c_foundation.h"

#include "myServerpacket.h"

LT_ServerParam serverParam;

LT_Server * pServer;


void initServer()
{
    lt_initial();

    serverParam.listenPort = 9001;
    serverParam.maxClient = 10;
    serverParam.maxRequestSize = 0;
    serverParam.maxProcessThreadSize = 5;
  
    serverParam.parseFunc = myServerParseFunc;
    serverParam.parseStructAllocFunc = myServerParseStructAllocFunc;
    serverParam.parseStructFreeFunc = myServerParseStructFreeFunc;
    serverParam.serialFunc = myServerSerialFunc;
    serverParam.requestDataFreeFunc = myServerRequestDataFreeFunc;
    serverParam.replyDataFreeFunc = myServerReplyDataFreeFunc;
    serverParam.processFunc = myServerProcessFunc;

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

