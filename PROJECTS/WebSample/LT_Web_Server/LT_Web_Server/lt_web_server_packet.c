/* 
 * lt_web_server_packet.c
 * write by zouql 20160422
 * zouqinglei@163.com
 * packet 
 * 
 */

#include "lt_web_server_packet.h"
  

/*
    if success, return 0;
    else return LT_ERR_PACKET
*/

static void url_encode(const char *src, char *dst, size_t dst_len) {
  static const char *dont_escape = "._-$,;~()";
  static const char *hex = "0123456789abcdef";
  const char *end = dst + dst_len - 1;

  for (; *src != '\0' && dst < end; src++, dst++) {
    if (isalnum(*(unsigned char *) src) ||
        strchr(dont_escape, * (unsigned char *) src) != NULL) {
      *dst = *src;
    } else if (dst + 2 < end) {
      dst[0] = '%';
      dst[1] = hex[(* (unsigned char *) src) >> 4];
      dst[2] = hex[(* (unsigned char *) src) & 0xf];
      dst += 2;
    }
  }

  *dst = '\0';
}

static size_t url_decode(const char *src, size_t src_len, char *dst,
                         size_t dst_len, int is_form_url_encoded) {
  size_t i, j;
  int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
    if (src[i] == '%' &&
        isxdigit(* (unsigned char *) (src + i + 1)) &&
        isxdigit(* (unsigned char *) (src + i + 2))) {
      a = tolower(* (unsigned char *) (src + i + 1));
      b = tolower(* (unsigned char *) (src + i + 2));
      dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
      i += 2;
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

  dst[j] = '\0'; /* Null-terminate the destination */

  return j;
}

static int GetContentLength(json_object * obj)
{
    json_object * headObj;
    json_object * lenObj;

    headObj = joo_get(obj, "head");

    if(headObj == NULL)
        return 0;

    lenObj = joo_get(headObj, "Content-Length");

    if(lenObj)
        return jo_get_int(lenObj);
    else
        return 0;
}

static const char * GetRequestMethod(json_object * obj)
{
    json_object * reqObj;

    reqObj = joo_get(obj, "req");
    return joo_get_string(reqObj, "method");
}

json_object * parseGetVars(char * pVars)
{
    char ** argv;
    int i, num;
    char * pValue;
    json_object * varObj;

    varObj = jo_new_object();

    num = lt_makeargv(pVars, "&", &argv);

    for(i = 0; i < num; i++)
    {
        pValue = strchr(argv[i], '=');
        if(pValue)
        {
            *pValue = '\0';
            pValue++;
            joo_set_string(varObj, argv[i], pValue);
        }
    }

    lt_freemakeargv(argv);

    return varObj;
}

boolean parseRequestLine(LTWeb_ParseStruct *parseSt)
{
    char ** argv;
    int num;
    json_object * reqObj;
    json_object * varObj;
    char * pVars;
    char decodeUri[1024];

    num = lt_makeargv(parseSt->requestline, " ", &argv);

    if(num != 3)
        return FALSE;

    reqObj = jo_new_object();

    joo_set_string(reqObj, "method", argv[0]);

    url_decode(argv[1], strlen(argv[1]), decodeUri, 1024, 1);

    if(strcmp(argv[0], "GET") == 0)
    {
        //exist '?'
        pVars = strchr(decodeUri, '?');
        if(pVars == NULL)
        {
            joo_set_string(reqObj, "uri", decodeUri);
        }
        else
        {
            *pVars = '\0';
            pVars++;
            joo_set_string(reqObj, "uri", decodeUri);
            varObj = parseGetVars(pVars);
            joo_add(reqObj, "var", varObj);
        }
    }
    else
    {
        joo_set_string(reqObj, "uri", decodeUri);
    }
    joo_set_string(reqObj, "ver", argv[2]);

    joo_add(parseSt->requestObj, "req", reqObj);

    lt_freemakeargv(argv);

    return TRUE;
}

boolean parseHeadBuf(LTWeb_ParseStruct *parseSt)
{
    char ** argv;
    int i, num;
    json_object * headObj;
    char * pValue;

    num = lt_makeargv(parseSt->headbuf, "\r\n", &argv);
    headObj = jo_new_object();

    for(i = 0; i < num; i++)
    {
        pValue = strchr(argv[i], ':');
        if(pValue)
        {
            *pValue = '\0';
            pValue += 2;
            joo_set_string(headObj, argv[i], pValue);
        }
    }

    joo_add(parseSt->requestObj, "head", headObj);

    lt_freemakeargv(argv);

    return TRUE;
}

void buildRequestObj(LT_Server * pServer, LT_ClientNode * pClientNode, LTWeb_ParseStruct * parseSt)
{
    if(parseSt->body != NULL)
    {
        joo_set_string(parseSt->requestObj, "body", parseSt->body);
        free(parseSt->body);
        parseSt->body = NULL;
    }

    LT_Server_Append(pServer, pClientNode, 0, parseSt->requestObj);

    parseSt->requestObj = NULL;   
}


/*
 * server_packet_parse_func()
 * return 
 *        0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */
int  ltwebServerParseFunc(const char * recvBuff, int recvSize, LT_Server * pServer, LT_ClientNode * pClientNode)
{
    LTWeb_ParseStruct * parseSt;
    char * pRecv;
    int parseSize;
    int restSize;
    int count;
    const char * pCRLF;
    const char * p2xCRLF;
    int nContentLength;
    const char * method;

    parseSt = (LTWeb_ParseStruct * )LT_Server_GetParseStruct(pClientNode);
    pRecv = (char *)recvBuff;
    parseSize = 0;
    restSize = recvSize; 

    while(restSize > 0)
    {
        if(parseSt->state == LTWEB_PARSE_REQLINE)
        {
            if(parseSt->requestObj == NULL)
            {
                parseSt->requestObj = jo_new_object();
            }
            
            pCRLF = memstr(pRecv, restSize, "\r\n");
            if(pCRLF)
            {
                count = pCRLF - pRecv + 2;
                BCOPY(pRecv, &parseSt->requestline[parseSt->index], count);
                parseSt->index += count;
                parseSt->requestline[parseSt->index] = '\0';

                pRecv += count;
                restSize -= count;

                parseSt->state = LTWEB_PARSE_HEAD;
                parseSt->index = 0;

                if(parseRequestLine(parseSt) == FALSE)
                    return  LT_ERR_PACKET;
            }
            else
            {
                BCOPY(pRecv, &parseSt->requestline[parseSt->index], restSize);
                parseSt->index += restSize;
                return LT_ERR_BUF;
            }
        }
        else if(parseSt->state == LTWEB_PARSE_HEAD)
        {
            p2xCRLF = memstr(pRecv, restSize, "\r\n\r\n");
            if(p2xCRLF)
            {
                count = p2xCRLF - pRecv + 4;
                BCOPY(pRecv, &parseSt->headbuf[parseSt->index], count);
                parseSt->index += count;
                parseSt->headbuf[parseSt->index] = '\0';

                pRecv += count;
                restSize -= count;

                parseSt->state = LTWEB_PARSE_BODY;
                parseSt->index = 0;

                if(parseHeadBuf(parseSt) == FALSE)
                    return  LT_ERR_PACKET;

                method = GetRequestMethod(parseSt->requestObj);
          
                nContentLength = GetContentLength(parseSt->requestObj);

                if((strcmp(method, "GET") == 0) || (nContentLength == 0)) //over
                {
                    parseSt->state = LTWEB_PARSE_REQLINE;
                    parseSt->body = NULL;

                    buildRequestObj(pServer, pClientNode, parseSt);
                }
                else
                {
                    parseSt->body = (char *)malloc(nContentLength + 1);
                }
            }
            else
            {
                BCOPY(pRecv, &parseSt->headbuf[parseSt->index], restSize);
                parseSt->index += restSize;
                return LT_ERR_BUF;
            }
        }
        else if(parseSt->state == LTWEB_PARSE_BODY)
        {
            if((parseSt->index + restSize) < nContentLength)
            {
                BCOPY(pRecv,&parseSt->body[parseSt->index],restSize);
                parseSt->index += restSize;
                return LT_ERR_BUF;
            }
            else
            {
                count = nContentLength - parseSt->index;
                BCOPY(pRecv, &parseSt->body[parseSt->index], count);
                pRecv += count;
                restSize -= count;

                parseSt->body[nContentLength] = '\0';

                //get a requestdata
                buildRequestObj(pServer, pClientNode, parseSt);
                
                parseSt->state = LTWEB_PARSE_REQLINE;

                
                parseSt->index = 0;
            }
        }
    }

    return 0;
}



void * ltwebServerParseStructAllocFunc()
{
    LTWeb_ParseStruct * parseStruct;

    parseStruct = (LTWeb_ParseStruct *)malloc(sizeof(LTWeb_ParseStruct));

    parseStruct->state = LTWEB_PARSE_REQLINE;
    parseStruct->body = NULL;
    parseStruct->index = 0;

    parseStruct->requestObj = NULL;

    return parseStruct;
}

void ltwebServerParseStructFreeFunc(void * data)
{
    LTWeb_ParseStruct * parseStruct;

    parseStruct = (LTWeb_ParseStruct *)data;
    if(parseStruct)
    {
        if(parseStruct->body != NULL)
        {
            free(parseStruct->body);
        }

        free(parseStruct);
    }
}

/*
 * server_packet_packet_func()
 *
 * return 0: if packet success
 * else is error
 */

int  ltwebServerSerialFunc(LT_RequestInfo * pInfo, char ** buf, int * buflen)
{
    json_object * replyObj;
    json_object * headObj;
    int headlen;
    int bodylen;
   
    char head[256];
    char status_head[1024];

    const char * pBody;

    

    replyObj = (json_object *)pInfo->replyData;

    sprintf(status_head, "HTTP/1.1 %s %s\r\n", 
        joo_get_string(replyObj, "status"),
        joo_get_string(replyObj, "reason"));
    
    headObj = joo_get(replyObj, "head");
    {
        json_object_object_foreach(headObj, key, val)
        {
            sprintf(head, "%s: %s\r\n", key, jo_get_string(val));
            strcat(status_head, head);
        }
    }

    strcat(status_head, "\r\n");

    headlen = strlen(status_head);
    bodylen = joo_get_int(headObj, "Content-Length");
    
    *buflen = headlen + bodylen;

    *buf = (char *)malloc(*buflen);
    
    BCOPY(status_head, *buf, headlen);
    pBody = joo_get_string(replyObj, "body");
    BCOPY(pBody, *buf + headlen, bodylen);

    return 0;
}


/*
 * server_free_data_func()
 *
 * 
 */

void ltwebServerRequestDataFreeFunc(void * data)
{
    jo_put((json_object *)data);
}

void ltwebServerReplyDataFreeFunc(void * data)
{
    jo_put((json_object *)data);
}

/*
 * server_process_func()
 * process the pInfo->requestdata and reply to the replydata

void ltwebServerProcessFunc(LT_RequestInfo * pInfo)
{

}

 */
