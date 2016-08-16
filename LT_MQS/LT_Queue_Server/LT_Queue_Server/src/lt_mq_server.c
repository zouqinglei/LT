
#include "lt_mq_server.h"
#include "lt_mq_server_packet.h"
#include "lt_mq_queue.h"
#include "lt_methods.h"

/************************************************************
 *    global data declare
 ************************************************************/
static LT_Server * pServer;
extern struct json_object * g_configObj;

extern int LT_Server_clientinfo(LT_Server * pServer, json_object ** record);


/************************************************************
 *    global data declare
 ************************************************************/
static void server_packet_process(LT_RequestInfo * pInfo)
{
    json_object * requestObj;
    json_object * replyObj;
    const char * reqName;
    

    if(pInfo->ltEvent == LT_SERVER_EVENT_CONNECT)
    {
        printf("client connect: %s:%d\n", inet_ntoa(pInfo->remote.sin_addr), ntohs(pInfo->remote.sin_port));
        return;
    }
    else if(pInfo->ltEvent == LT_SERVER_EVENT_DISCONNECT)
    {
        printf("client disconnect: %s:%d\n", inet_ntoa(pInfo->remote.sin_addr), ntohs(pInfo->remote.sin_port));
        return;
    }

    requestObj = (json_object *)pInfo->requestData;
    reqName = joo_get_string(requestObj,"req");

    replyObj = jo_new_object();
    joo_set_string(replyObj, "errcod", "SUCCESS");
    joo_set_string(replyObj, "errstr","");
    pInfo->replyData = (void *)replyObj;

    lt_methods_exec(reqName, pInfo);

    return;
}
/*
    ltmq_create_queue()
    requestData object:
    {
        "req":'QUEUE.CREATE',
        "params": {
            "queue_name":"xxxx",
            "persistent":"Y" or "N",
            "max_msgnum":0 default 99999,
            "max_msgsize":0 default 4M,
            "remark":""
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":"The queue created success."
     }
*/
static void svc_ltmq_client_info(LT_RequestInfo * pInfo)
{
    int ret;
    json_object * replyDataObj;
    json_object * dataObj;

    replyDataObj = (json_object *)pInfo->replyData;
       
    

    ret = LT_Server_clientinfo(pServer, &dataObj);
    if(ret == 0)
    {
        joo_set_string(replyDataObj, "errcod", "SUCCESS");
        joo_set_string(replyDataObj, "errstr", "");
        joo_add(replyDataObj, "data", dataObj);
    }
    else
    {
        joo_set_string(replyDataObj, "errcod", "ERR_FAIL");
        joo_set_string(replyDataObj, "errstr", "");
    }

    
    return;
}

static int init_method()
{
    lt_methods_register( "CLIENT.INFO", svc_ltmq_client_info);
    return 0;
}



int ltmq_server_init()
{
    struct json_object * globalObj;
    LT_ServerParam param;

    init_method();

    globalObj = joo_get(g_configObj, "global");
    memset(&param,0, sizeof(LT_ServerParam));

    param.listenPort = (unsigned short)lt_config_getInt(globalObj, "serverport",8100);
    param.maxClient = (unsigned short)lt_config_getInt(globalObj, "maxclient",10);
    param.maxRequestSize = (unsigned short)lt_config_getInt(globalObj, "maxrequestsize",0);
    param.maxWorkThreadSize = (unsigned short)lt_config_getInt(globalObj, "maxworkthreads",10);
  
    param.parseFunc = ltmqServerParseFunc;
    param.parseStructAllocFunc = ltmqServerParseStructAllocFunc;
    param.parseStructFreeFunc = ltmqServerParseStructFreeFunc;
    param.serialFunc = ltmqServerSerialFunc;
    param.requestDataFreeFunc = ltmqServerRequestDataFreeFunc;
    param.replyDataFreeFunc = ltmqServerReplyDataFreeFunc;
    param.processFunc = server_packet_process;

	pServer = LT_Server_New(&param);
	return LT_Server_Start(pServer);
}

int ltmq_server_destroy()
{   
	LT_Server_Stop(pServer);

	LT_Server_Free(pServer);

    return 0;
}

LT_Server * ltmq_server_instance()
{
    return pServer;
}