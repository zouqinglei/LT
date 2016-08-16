#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lt_c_foundation.h"
#include "lt_web_client_packet.h"


/**************************************************
 * global variable
 *************************************************/

static LT_Client * pClient;

/*************************************************
 * local function 
 ************************************************/

/***************************************************
* module service
***************************************************/

int Http_Get(const char * hostIP, short port, const char * uri, json_object * varObj, json_object ** resultObj, unsigned int waitseconds)
{
    int ret;
    json_object * reqObj;
    json_object * headObj;
    json_object * requestObj;

    pClient = LT_Client_New(hostIP, port, 0, 1, 0, 
                            ltwebClientParseFunc,
                            ltwebClientParseStructAllocFunc,
                            ltwebClientParseStructFreeFunc,
                            ltwebClientSerialFunc,
                            ltwebClientRequestDataFreeFunc,
                            ltwebClientReplyDataFreeFunc);

    if(!pClient) 
        return LT_ERR_FAIL;

    ret = LT_Client_Start(pClient);
    if(ret != 0)
    {
        LT_Client_Free(pClient);
        return ret;
    }

    reqObj = jo_new_object();
    joo_set_string(reqObj, "ver", "HTTP/1.1");
    joo_set_string(reqObj, "method", "GET");
    joo_set_string(reqObj, "uri", uri);
    joo_add(reqObj, "var", jo_get(varObj));

    headObj = jo_new_object();
    joo_set_string(headObj, "Accept", "text/html");
    
    requestObj = jo_new_object();

    joo_add(requestObj, "req", reqObj);
    joo_add(requestObj, "head", headObj);
    joo_set_string(requestObj, "body", "");

    ret = LT_Client_Invoke(pClient, requestObj, resultObj,  waitseconds);
    jo_put(requestObj);

    LT_Client_Stop(pClient);
    LT_Client_Free(pClient);

    return ret;
}


int main(int argc, char *argv[])
{
    int ret;
    json_object * varObj;
    json_object * resultObj;
    const char * status;
    const char * reason;
    const char * html;

    lt_initial();

    varObj = jo_new_object();
    joo_set_string(varObj, "username", "admin");
    joo_set_string(varObj, "password", "ok");

    ret = Http_Get("127.0.0.1", 8000, "/login", varObj, &resultObj, 15);
    
    jo_put(varObj);

    if(ret == 0)
    {
        status = joo_get_string(resultObj, "status");
        reason = joo_get_string(resultObj, "reason");
        if(strcmp(status, "200") == 0)
        {
            html = joo_get_string(resultObj, "body");
            printf("%s\n", html);
        }
        else
        {
            printf("%s %s\n", status, reason);
        }

        jo_put(resultObj);
    }
    else
    {
        printf("An error occured: %d.\n", ret);
    }

    getchar();

    return 0;
}
