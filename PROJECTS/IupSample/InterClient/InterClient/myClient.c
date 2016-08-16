#include "stdio.h"
#include "stdlib.h"
#include "myClient.h"

#include "lt_c_foundation.h"

#include "myClientPacket.h"
#include "myJsonClientPacket.h"

#include "myDialog.h"


LT_Client * pClient;

LT_Thread * pThread;


#define MY_JSON_PARSE 0

#ifndef MY_JSON_PARSE 

pthread_handler_decl(handle_recv,arg)
{
    int ret;
    int requestID;
    ReplyData * pReplyData;

	while(!pThread->abort_loop)
    {
        ret = LT_Client_Recv(pClient, &requestID, &pReplyData, 3);
        if(ret == 0)
        {
            dlgShowRecvMessage(pReplyData->name, pReplyData->message);
            myClientReplyDataFreeFunc(pReplyData);
        }
    }

    LT_Thread_exit(pThread);
}

#else

pthread_handler_decl(handle_recv,arg)
{
    int ret;
    int requestID;
    char  * pReplyData;
    json_object * packetObj;
    const char * name;
    const char * message;

	while(!pThread->abort_loop)
    {
        ret = LT_Client_Recv(pClient, &requestID, &pReplyData, 3);
        if(ret == 0)
        {
            packetObj = json_tokener_parse(pReplyData);
            name = joo_get_string(packetObj, "name");
            message = joo_get_string(packetObj, "message");
            dlgShowRecvMessage(name, message);
            myJsonClientReplyDataFreeFunc(pReplyData);
            jo_put(packetObj);
        }
    }

    LT_Thread_exit(pThread);
}

#endif



void clientInit(const char * hostIP, unsigned short port)
{
    lt_initial();
#ifndef MY_JSON_PARSE 
    pClient = LT_Client_New(hostIP, port, 0,0,0,
        myClientParseFunc,
        myClientParseStructAllocFunc,
        myClientParseStructFreeFunc,
        myClientSerialFunc,
        myClientRequestDataFreeFunc,
        myClientReplyDataFreeFunc);
#else
    pClient = LT_Client_New(hostIP, port, 0,0,0,
        myJsonClientParseFunc,
        myJsonClientParseStructAllocFunc,
        myJsonClientParseStructFreeFunc,
        myJsonClientSerialFunc,
        myJsonClientRequestDataFreeFunc,
        myJsonClientReplyDataFreeFunc);

#endif
    pThread = LT_Thread_new(handle_recv, NULL);
}

void clientConnect()
{
    LT_Client_Start(pClient);
    LT_Thread_start(pThread);
}

void clientDisConnect()
{
    LT_Client_Stop(pClient);
    LT_Thread_stop(pThread);
}

void clientDestroy()
{
    LT_Client_Free(pClient);
    LT_Thread_free(pThread);
}

#ifndef MY_JSON_PARSE
void clientSend(const char * name, const char * message)
{
    RequestData * pRequestData;

    pRequestData = (RequestData * )malloc(sizeof(RequestData));
    pRequestData->name = (char *)name;
    pRequestData->message = (char *)message;
    LT_Client_Send(pClient, 0, pRequestData);

    free(pRequestData);
}

#else

void clientSend(const char * name, const char * message)
{
    const char * pRequestData;
    json_object * packetObj;

    packetObj = jo_new_object();

    joo_set_string(packetObj, "name", name);
    joo_set_string(packetObj, "message", message);

    pRequestData = jo_to_json_string(packetObj);

    LT_Client_Send(pClient, 0, pRequestData);

    jo_put(packetObj);
}
#endif

