/* 
 * lt_web_server_packet.h
 * write by zouql 20160422
 * zouqinglei@163.com
 * packet 
 * 
 */

#include "lt_c_foundation.h"


#ifndef LT_WEB_SERVER_PACKET_H
#define LT_WEB_SERVER_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

/*
request object:
{
    "ori": { "reqline" : "GET /login?user=xxx&pass=xxx HTTP/1.1(CRLF)",
             "head" : " Accept:text (CRLF)
                        ...
                        Content-Length:22 (CRLF)
                        Connection:Keep-Alive (CRLF)",
             "body": "xxxx" 
            },

    "req": {
              "ver": "HTTP/1.1",
              "method": "GET",
              "uri": "/login",
              "var": {
                      "user":"xxx",
                      "pass":"xxx",
                     }
              
           },
    "head": {
                "Accept": "text",
                "Content-Length":22,
                ...
            },
    "body": "xxxx",
}

reply object:
{
    "status": "200",
    "reason": "OK",
    "head": {
                "Content-Length":22,
                "Content-Type": "text/html",
                ...
            },
    "body": "xxx"
}
*/

typedef struct stLTWeb_ParseStruct
{
    int state;

    char requestline[1024];
    char headbuf[1024];
    char * body;

    int index;

    json_object * requestObj;

}LTWeb_ParseStruct;

#define LTWEB_PARSE_REQLINE 0
#define LTWEB_PARSE_HEAD 1
#define LTWEB_PARSE_BODY 2

/*
 * server_packet_parse_func()
 * return 
 *        0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */
int  ltwebServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode);



void * ltwebServerParseStructAllocFunc();

void ltwebServerParseStructFreeFunc(void * data);

/*
 * server_packet_packet_func()
 *
 * return 0: if packet success
 * else is error
 */

int  ltwebServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen);

/*
 * server_free_data_func()
 *
 * 
 */

void ltwebServerRequestDataFreeFunc(void * data);

void ltwebServerReplyDataFreeFunc(void * data);

/*
 * server_process_func()
 * process the pInfo->requestdata and reply to the replydata
 */
//void ltwebServerProcessFunc(LT_RequestInfo * pInfo);


#ifdef __cplusplus
}
#endif

#endif /*LT_WEB_SERVER_PACKET_H*/
