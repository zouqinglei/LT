#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lt_c_foundation.h"
#include "lt_web_server_packet.h"


/**************************************************
 * global variable
 *************************************************/

static LT_Server * pServer;

/*************************************************
 * local function 
 ************************************************/

/***************************************************
* module service
***************************************************/

const char * htmlTemplate = "<html> \
                    <head> \
                    <title>Hello World!</title> \
                    </head> \
                    <body> \
                    <h1> This is a sample of LT_WEB. <br> \
                    <p>Your name is %s and PassWord is %s </p>\
                    </body> \
                    </html>";

const char * html404 = "<html> \
                    <head> \
                    <title>404 Not Found</title> \
                    </head> \
                    <body> \
                    <h1> Error This Page Not Found. <br> \
                    </body> \
                    </html>";

/*
 * server_process_func()
 * process the pInfo->requestdata and reply to the replydata
 */
void ltwebServerProcessFunc(LT_RequestInfo * pInfo)
{
    json_object * reqObj;
    json_object * varObj;
    json_object * replyObj;
    json_object * headObj;
    char html[1024];
    const char * method;
    const char * uri;

    if(pInfo->ltEvent != LT_SERVER_EVENT_REQUEST)
    {
        return;
    }

    replyObj = jo_new_object();
    pInfo->replyData = (void *)replyObj;

    reqObj = joo_get((json_object*)pInfo->requestData, "req");
    method = joo_get_string(reqObj, "method");
    uri = joo_get_string(reqObj, "uri");
    varObj = joo_get(reqObj, "var");

    if(strcmp(method, "GET") == 0)
    {
        if(strcmp(uri, "/login") == 0)
        {
            sprintf(html, htmlTemplate, 
                joo_get_string(varObj, "username"),
                joo_get_string(varObj, "password"));
            
            joo_set_string(replyObj, "status", "200");
            joo_set_string(replyObj, "reason", "OK");

            headObj = jo_new_object();
            joo_set_string(headObj, "Content-Type", "text/html");
            joo_set_int(headObj, "Content-Length", strlen(html));
            joo_add(replyObj, "head", headObj);
            joo_set_string(replyObj, "body", html);
            return;
        }
    }

    //404 not found
    joo_set_string(replyObj, "status", "400");
    joo_set_string(replyObj, "reason", "Not Found");
    

    headObj = jo_new_object();
    joo_set_string(headObj, "Content-Type", "text/html");
    joo_set_int(headObj, "Content-Length", strlen(html404));
    joo_add(replyObj, "head", headObj);
    joo_set_string(replyObj, "body", html404); 
}

int ltweb_server_init()
{
    LT_ServerParam param;

    param.listenPort = 8000;
    param.maxClient = 20;
    param.maxRequestSize = 0;
    param.maxWorkThreadSize = 10;
  
    param.parseFunc = ltwebServerParseFunc;
    param.parseStructAllocFunc = ltwebServerParseStructAllocFunc;
    param.parseStructFreeFunc = ltwebServerParseStructFreeFunc;
    param.serialFunc = ltwebServerSerialFunc;
    param.requestDataFreeFunc = ltwebServerRequestDataFreeFunc;
    param.replyDataFreeFunc = ltwebServerReplyDataFreeFunc;
    param.processFunc = ltwebServerProcessFunc;

	pServer = LT_Server_New(&param);
	return LT_Server_Start(pServer);
}

int ltweb_server_destroy()
{   
	LT_Server_Stop(pServer);

	LT_Server_Free(pServer);

    return 0;
}





int main(int argc, char *argv[])
{
 
    lt_initial();

    ltweb_server_init();

    getchar();

    ltweb_server_destroy();

    return 0;
}
