/* 
 * lt_web_client_packet.c
 * write by zouql 20160428
 * zouqinglei@163.com
 * 
 * 
 */

#include "lt_web_client_packet.h"
  

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





boolean parseStatusLine(LTWeb_ParseStruct *parseSt)
{
    char ** argv;
    int num;

    num = lt_makeargv(parseSt->statusline, " ", &argv);

    if(num != 3)
        return FALSE;

    joo_set_string(parseSt->replyObj, "status", argv[1]);
    joo_set_string(parseSt->replyObj, "reason", argv[2]);

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

    joo_add(parseSt->replyObj, "head", headObj);

    lt_freemakeargv(argv);

    return TRUE;
}

void buildReplyObj(LT_Client * pClient, LTWeb_ParseStruct * parseSt)
{
    if(parseSt->body != NULL)
    {
        joo_set_string(parseSt->replyObj, "body", parseSt->body);
        free(parseSt->body);
        parseSt->body = NULL;
    }

    LT_Client_Append(pClient, 0, parseSt->replyObj);

    parseSt->replyObj = NULL;   
}


/*
 * client_packet_parse_func()
 * return 
 *        0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */
int  ltwebClientParseFunc(const char * recvBuff, int recvSize, LT_Client * pClient,
                                void * parseStruct)
{
    LTWeb_ParseStruct * parseSt;
    char * pRecv;
    int parseSize;
    int restSize;
    int count;
    const char * pCRLF;
    const char * p2xCRLF;
    int nContentLength;

    parseSt = (LTWeb_ParseStruct * )parseStruct;
    pRecv = (char *)recvBuff;
    parseSize = 0;
    restSize = recvSize; 

    while(restSize > 0)
    {
        if(parseSt->state == LTWEB_PARSE_STATUSLINE)
        {
            if(parseSt->replyObj == NULL)
            {
                parseSt->replyObj = jo_new_object();
            }
            
            pCRLF = memstr(pRecv, restSize, "\r\n");
            if(pCRLF)
            {
                count = pCRLF - pRecv + 2;
                BCOPY(pRecv, &parseSt->statusline[parseSt->index], count);
                parseSt->index += count;
                parseSt->statusline[parseSt->index] = '\0';

                pRecv += count;
                restSize -= count;

                parseSt->state = LTWEB_PARSE_HEAD;
                parseSt->index = 0;

                if(parseStatusLine(parseSt) == FALSE)
                    return  LT_ERR_PACKET;
            }
            else
            {
                BCOPY(pRecv, &parseSt->statusline[parseSt->index], restSize);
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

                nContentLength = GetContentLength(parseSt->replyObj);

                if((nContentLength == 0)) //over
                {
                    parseSt->state = LTWEB_PARSE_STATUSLINE;
                    parseSt->body = NULL;

                    buildReplyObj(pClient, parseSt);
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
                buildReplyObj(pClient, parseSt);
                
                parseSt->state = LTWEB_PARSE_STATUSLINE;
                parseSt->index = 0;
            }
        }
    }

    return 0;
}



void * ltwebClientParseStructAllocFunc()
{
    LTWeb_ParseStruct * parseStruct;

    parseStruct = (LTWeb_ParseStruct *)malloc(sizeof(LTWeb_ParseStruct));

    parseStruct->state = LTWEB_PARSE_STATUSLINE;
    parseStruct->body = NULL;
    parseStruct->index = 0;

    parseStruct->replyObj = NULL;

    return parseStruct;
}

void ltwebClientParseStructFreeFunc(void * data)
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
 * client_packet_packet_func()
 *
 * return 0: if packet success
 * else is error
 */

int  ltwebClientSerialFunc(const unsigned int requestID, const void * requestData, char ** buf, int * buflen)
{
    json_object * reqObj;
    json_object * varObj;
    json_object * headObj;
    boolean bFirst;
    int headlen;
   
    char tmp[256];
    char var[1024];
    char uri[1024];
    char encodeUri[1024];
    char request_head[2048];

    reqObj = joo_get((json_object *)requestData, "req");

    varObj = joo_get(reqObj, "var");
    var[0] = '\0';
    bFirst = TRUE;    
    {
        json_object_object_foreach(varObj, key, val)
        {
            if(bFirst)
            {
                sprintf(tmp, "%s=%s", key, jo_get_string(val));
                bFirst = FALSE;
            }
            else
            {
                sprintf(tmp, "&%s=%s", key, jo_get_string(val));
            }
            strcat(var, tmp);
        }
    }
    
    if(strlen(var) == 0)
    {
        sprintf(uri, "%s", joo_get_string(reqObj, "uri"));
    }
    else
    {
        sprintf(uri, "%s?%s", joo_get_string(reqObj, "uri"), var);
    }

    url_encode(uri, encodeUri, 1024);

    sprintf(request_head, "%s %s HTTP/1.1\r\n", 
            joo_get_string(reqObj, "method"),
            encodeUri);
    
    headObj = joo_get((json_object *)requestData, "head");
    {
        json_object_object_foreach(headObj, key, val)
        {
            sprintf(tmp, "%s: %s\r\n", key, jo_get_string(val));
            strcat(request_head, tmp);
        }
    }

    strcat(request_head, "\r\n");

    headlen = strlen(request_head);
    *buflen = headlen;
    *buf = (char *)malloc(*buflen);
    BCOPY(request_head, *buf, headlen);

    return 0;
}


/*
 * server_free_data_func()
 *
 * 
 */

void ltwebClientRequestDataFreeFunc(void * data)
{
    jo_put((json_object *)data);
}

void ltwebClientReplyDataFreeFunc(void * data)
{
    jo_put((json_object *)data);
}

