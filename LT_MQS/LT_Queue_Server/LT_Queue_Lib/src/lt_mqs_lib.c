
#include "lt_mqs.h"
#include "lt_c_foundation.h"
#include "lt_mq_client_packet.h"



/*
    check_queue_name(const char * queue_name)

    if ok then return 0,
    else return -1
*/
static int check_queue_name(const char * queue_name)
{
    int i;
    int len;

    if(queue_name == NULL)
        return -1;

    if(strlen(queue_name) == 0)
        return -1;

    if(isalpha(queue_name[0]) == 0)
        return -1;

    len = strlen(queue_name);
    for(i = 1; i < len; i++)
    {
        if(isalnum(queue_name[i]) == 0 && queue_name[i] != '_')
        {
            return -1;
        }
    }

    return 0;
}


/* interface */
LTMQHandle  ltmq_init(const char * hostIP, unsigned short port, int flag)
{
    int ret;
    LT_Client * pClient;

    if(port == 0)
        port = 8100;
    
    pClient = LT_Client_New(hostIP, 
                          port,						  
						  flag,
                          1, 
	                      0,	 
                          ltmqClientParseFunc,
                          ltmqClientParseStructAllocFunc,
                          ltmqClientParseStructFreeFunc,
                          ltmqClientSerialFunc,
                          ltmqClientRequestDataFreeFunc,
						  ltmqClientReplyDataFreeFunc);


    if(pClient)
    {
        ret = LT_Client_Start(pClient);
        if(ret != 0)
        {
            LT_Client_Free((LT_Client *)pClient);
            pClient = NULL;
        }
    }

    return pClient;
}

int ltmq_term(LTMQHandle hConn)
{
    if(hConn)
    {
        LT_Client_Stop((LT_Client *)hConn);
        LT_Client_Free((LT_Client *)hConn);
    }
    return 0;
}


static int errcod2int(const char * errcod)
{
    int ret;

    if(strcmp(errcod, "SUCCESS") == 0)
    {
        ret = 0;
    }
    else if(strcmp(errcod, "ERR_NOENT") == 0) 
    {
        ret = LT_ERR_NOENT;
    }
    else if(strcmp(errcod, "ERR_NODATA") == 0)
    {
        ret = LT_ERR_NODATA;
    }
    else if(strcmp(errcod, "ERR_DENIED") == 0)
    {
        ret = LT_ERR_DENIED;
    }
    else if(strcmp(errcod, "ERR_EXIST") == 0)
    {
        ret = LT_ERR_EXIST;
    }
    else if(strcmp(errcod, "ERR_RETRY") == 0)
    {
        ret = LT_ERR_RETRY;
    }
    else
    {
        ret = -1;
    }

    return ret;
}


/*
request
     {
        "req":"QUEUE.CREATE",
        "param":
        {
            "queue_name":"queue name",
            "persistent":"Y" or "N",
            "max_msgnum":0 default 99999,
            "max_msgsize":0 default 4M,
            "remark":""
        }         
     }

reply:
     headobj
     {
        "errcod":"SUCCESS",
        "errstr":"The queue created success."
        "data": {}    
     }
*/

int ltmq_create(LTMQHandle hConn, 
                 const char * queue_name, 
                 int persistent,
                 int max_msgnum,
                 int max_msgsize,
                 const char * remark)
{
    int ret;
    
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;
    

    const char * errcod;
    char * strQueueName;

    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }


    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    joo_set_string(paramsObj, "persistent", persistent?"Y":"N");
    joo_set_int(paramsObj,    "max_msgnum", max_msgnum);
    joo_set_int(paramsObj,    "max_msgsize", max_msgsize);
    joo_set_string(paramsObj, "remark", (remark == NULL)?"":remark);
    
    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.CREATE");
    joo_add(requestObj, "params", paramsObj);

    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);
        jo_put(replyObj);

    }

    jo_put(requestObj);

    return ret;
}

/*
request
     {
        "req":"QUEUE.DELETE",
        "param":
        {
            "queue_name":"queue name",
        }         
     }

reply:
     headobj
     {
        "errcod":0, or other errcod in lt_const.h
        "errstr":"The queue created success."
        "data": {}
           
     }
*/
int ltmq_delete(LTMQHandle hConn,
                 const char * queue_name)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;

    const char * errcod;
    char * strQueueName;

    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }
    
    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.DELETE");
    joo_add(requestObj, "params", paramsObj);

    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);
        jo_put(replyObj);   
    }

    jo_put(requestObj);

    return ret;
}

/*      
   requestData object:
    {
        "req":'QUEUE.PUTMSG',
        "params": {
            "queue_name":"xxxx",
            "data":{
                "correlid":"xxx",
                "tag":"xxx", 
                "expiry":10, 
                "title":"xxx", 
                "content":"xxx"}
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":""
     }

*/
int ltmq_put(LTMQHandle hConn,
              const char * queue_name,
              json_object * msgObj)
{
    int ret;
    
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;

    const char * errcod;
    const char * errstr;
    char * strQueueName;

    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }

    if(msgObj == NULL)
    {
        return LT_ERR_PARAM;
    }
    
    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    
    joo_add(paramsObj, "data", msgObj);
    jo_get(msgObj);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.PUTMSG");
    joo_add(requestObj, "params", paramsObj);
    
    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        errstr = joo_get_string(replyObj, "errstr");
        ret = errcod2int(errcod);
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
}

         
/*
ltmq_get()
requestData object:
    {
        "req":'QUEUE.GETMSG',
        "params": {
            "queue_name":"xxxx",
            "correlid":"xxx",
            "tag":"xxx", 
            "waitseconds":5,
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":""
        "data":{
               "msgid":xxx,
               "correlid":xx,
               "tag":xx,
               "expiry":xx,
               "title":xx,
               "content":xx,
               "create_time":xx,
               }
     }

*/
int ltmq_get(LTMQHandle hConn,
             const char * queue_name,
             const char * correlid,
             const char * tag,
             json_object * msgObj,
             int waitseconds)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;
    struct json_object * dataObj;
    const char * errcod;
    const char * errstr;

    char * strQueueName;


    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }

    if(msgObj == NULL)
    {
        return LT_ERR_PARAM;
    }

    if(waitseconds <= 0)
    {
        waitseconds = 15;
    }

    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    joo_set_string(paramsObj, "correlid", correlid?correlid:"");
    joo_set_string(paramsObj, "tag", tag?tag:"");
    joo_set_int(paramsObj, "waitseconds",waitseconds);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.GETMSG");
    joo_add(requestObj, "params", paramsObj);
    
    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, waitseconds);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        errstr = joo_get_string(replyObj, "errstr");
      
        ret = errcod2int(errcod);
        if(ret == 0)
        {

            dataObj = joo_get(replyObj, "data");
            {
                json_object_object_foreach(dataObj, key, val)
                {
                    joo_add(msgObj, key, jo_get(val));
                }
            }
        }
        
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
}





/* 

    msg wrap function 
    {
        "msgid":"",
        "correlid":"",
        "tag":"",
        "expiry":-1,
        "title":"",
        "content":""
    }

*/


json_object * ltmq_msg_new(const char * correlid, 
                 const char * tag, 
                 int expiry, 
                 const char * title, 
                 const char * content)
{
    json_object * msgObj;

    msgObj = jo_new_object();
    
    joo_set_string(msgObj, "correlid", correlid?correlid:"");
    joo_set_string(msgObj, "tag", tag?tag:"");
    joo_set_int(msgObj,    "expiry", expiry);
    joo_set_string(msgObj, "title", title?title:"");
    joo_set_string(msgObj, "content", content?content:"");

    return msgObj;
}


/*
    recordset:
    "record": {"column":["a","b","c","d"],
     "data":[
             [  ],
             [  ]
            ]
    }
*/


/*
    type is : QUEUE, CLIENT, MODULE
    name is the type's single name or NULL or ""

    request obj:
    {
       "req":"",
       "params":
       {
           "name":xxx,
       
       }
    }

    reply obj:
    {
        "errcod":"SUCCESS",
        "errstr":"xxx",
        "data":{},
    }
*/

int ltmq_info(LTMQHandle hConn,
              const char * type,
              const char * name,
              json_object * recordObj)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;
    struct json_object * dataObj;
    const char * errcod;

    
    char * strTmp;

    if(type == NULL)
        return LT_ERR_PARAM;

    if(name && strlen(name) > 0)
    {
        if(check_queue_name(name) == -1)
        {
            return LT_ERR_PARAM;
        }
    }

    paramsObj = jo_new_object();

    if(name == NULL || strlen(name) == 0)
    {
        joo_set_string(paramsObj, "name", "");
    }
    else
    {
        strTmp = strupr(STR_NEW(name));
        joo_set_string(paramsObj, "name", strTmp);
        free(strTmp);
    }

 
    requestObj = jo_new_object();
    if(strcmp(type, "QUEUE") == 0)
    {
        joo_set_string(requestObj, "req", "QUEUE.INFO");
    }
    else if(strcmp(type, "CLIENT") == 0)
    {
        joo_set_string(requestObj, "req", "CLIENT.INFO");
    }
    else if(strcmp(type, "MODULE") == 0)
    {
        joo_set_string(requestObj, "req", "NS.INFO");
    }
    else
    {
        joo_set_string(requestObj, "req", type);
    }
    
    joo_add(requestObj, "params", paramsObj);
    
    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);       
        if(ret == 0)
        {
            dataObj = joo_get(replyObj, "data");
            if(recordObj)
            {
                json_object_object_foreach(dataObj, key, val)
                {
                    joo_add(recordObj, key, jo_get(val));
                }
            }
            
        }
       
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
}





int ltmq_clear(LTMQHandle hConn,
               const char * queue_name)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;

    const char * errcod;
    char * strQueueName;

    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }
    
    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.CLEAR");
    joo_add(requestObj, "params", paramsObj);

    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
 
}

/*
    start: 
    limit:

     LIMIT  1   OFFSET 0



*/

int ltmq_browse(LTMQHandle hConn,
                const char * queue_name,
                int start,
                int limit,
                json_object * recordObj)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;
    struct json_object * dataObj;

    const char * errcod;
    char * strQueueName;

    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }
    
    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    joo_set_int(paramsObj, "start",start);
    joo_set_int(paramsObj, "limit",limit);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.BROWSE");
    joo_add(requestObj, "params", paramsObj);

    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);       
        if(ret == 0)
        {
            dataObj = joo_get(replyObj, "data");
            if(recordObj)
            {
                json_object_object_foreach(dataObj, key, val)
                {
                    joo_add(recordObj, key, jo_get(val));
                }
            }
            
        }
       
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
  
}
/*
  ltmq_browse_msg()
  {
        "req":'QUEUE.BROWSEMSG',
        "params": {
            "queue_name":"xxxx",
            "msgid":"xxx",
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":""
        "data":{
               "msgid":xxx,
               "correlid":xx,
               "tag":xx,
               "expiry":xx,
               "title":xx,
               "content":xx,
               "create_time":xx,
               }
     }
*/

int ltmq_browse_msg(LTMQHandle hConn,
                const char * queue_name,
                const char * msgid,
                json_object * msgObj)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;
    struct json_object * dataObj;
    const char * errcod;
    const char * errstr;

    char * strQueueName;

    if(msgid == NULL || strlen(msgid) == 0)
        return LT_ERR_PARAM;

    if(check_queue_name(queue_name) == -1)
    {
        return LT_ERR_PARAM;
    }

    if(msgObj == NULL)
    {
        return LT_ERR_PARAM;
    }

    paramsObj = jo_new_object();
  
    strQueueName = strupr(STR_NEW(queue_name));
    joo_set_string(paramsObj, "queue_name", strQueueName);
    free(strQueueName);

    joo_set_string(paramsObj, "msgid", msgid);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "QUEUE.BROWSEMSG");
    joo_add(requestObj, "params", paramsObj);
    
    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        errstr = joo_get_string(replyObj, "errstr");
      
        ret = errcod2int(errcod);
        if(ret == 0)
        {

            dataObj = joo_get(replyObj, "data");
            {
                json_object_object_foreach(replyObj, key, val)
                {
                    joo_add(msgObj, key, jo_get(val));
                }
            }
        }
        
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
}




/****************************************
* ltns 
****************************************/

/* interface */

/*
 *  task parameter initialize
 */
static void ltSvcInfo_clean(LtSvcInfo  * pInfo)
{
    if(pInfo->paramObj != NULL)
    {
        jo_put(pInfo->paramObj);
    }

    if(pInfo->resultObj != NULL)
    {
        jo_put(pInfo->resultObj);
    }

    memset(pInfo, 0, sizeof(LtSvcInfo));

    pInfo->reply = 1;
}

	

/*****************************************
 * lt name server func 
 ****************************************/

static json_object * g_dtbl = NULL;
static LTMQHandle g_hMQ = NULL;


void ltns_register(const char * service, LtSvcFunc fp)
{
    if(g_dtbl == NULL)
    {
        g_dtbl = jo_new_object();
    }

    if(service == NULL)
        return;
    
    joo_set_int(g_dtbl, service, (int)fp);
}

int ltns_serverloop(const char * module, int argc,char *argv[])
{
    json_object * msgObj;
    LtSvcInfo info;
    int abort_loop;
    int i;
    int ret;
    char * host;
    unsigned short port;
    const char * correlid;
    const char * service;
    const char * content;
    LtSvcFunc svc_fun;

    abort_loop = 0;
    g_hMQ = NULL;
    host = "127.0.0.1";
	port = 8100;
   
	/* analysise command line parameter */
	for (i = 1; i < argc; i++) 
	{
		if (argv[i][0] == '-') 
		{
			switch (argv[i][1]) 
			{
			case 'p':
				if(i < argc -1)
					port = atoi(argv[i + 1]);
				break;
			}
		}
	}

    lt_initial();
    g_hMQ = ltmq_init( host, port, 0);
    if(g_hMQ == NULL)
        return LT_ERR_CONNECT;

    memset(&info, 0, sizeof(LtSvcInfo));

    msgObj = jo_new_object();

	while(!abort_loop)
	{   
		ltSvcInfo_clean(&info);		
	    //receive msg
        ret = ltmq_get(g_hMQ, "LTNS_USER_REQUEST", NULL, module, msgObj, 5);
        if(ret == LT_ERR_RETRY)
        {
            ret = ltmq_get(g_hMQ, "LTNS_USER_REQUEST", NULL, module, msgObj, 5);
        }

        if(ret == LT_ERR_CONNECT)
            break;
        else if(ret != 0)
            continue;
       
        correlid = joo_get_string(msgObj, "correlid");
        service = joo_get_string(msgObj, "title");
        if(strcmp(correlid, "SYS") == 0)
        {
            if(strcmp(service, "STOP") == 0)
                break;
            else
                continue;
        }

        info.name = service;
        content = joo_get_string(msgObj, "content");
        info.paramObj = json_tokener_parse(content);
        
        info.resultObj = jo_new_object();
        joo_set_string(info.resultObj, "errcod", "SUCCESS");
        joo_set_string(info.resultObj, "errstr", "");

        if(g_dtbl)
        {
            svc_fun = (LtSvcFunc)joo_get_int(g_dtbl, service);
        }
        else
        {
            svc_fun = NULL;
        }

        if(svc_fun == NULL)
        {
            ret = LT_ERR_NOENT;
            joo_set_string(info.resultObj, "errcod", "ERR_NOENT");
            joo_set_string(info.resultObj, "errstr", "");
        }
        else
        {
            ret = 0;
			svc_fun(&info);
        }

        if(info.reply == 0)
            continue;

        //send result msg 
        if(ret == 0)
        {
            joo_set_string(msgObj, "title", "SUCCESS");
        }
        else
        {
            joo_set_string(msgObj, "title",  "ERR_NOENT");
        }

        joo_set_string(msgObj, "content", jo_to_json_string(info.resultObj));        
        ret = ltmq_put(g_hMQ, "LTNS_USER_REPLY", msgObj);
        if(ret == LT_ERR_CONNECT)
            break;
	}

    jo_put(msgObj);
    ltmq_term(g_hMQ);
    ltSvcInfo_clean(&info);
    jo_put(g_dtbl);
	
	return ret;
}

/*******************************************
* call
*****************************************/

int ltns_call_ex(LTMQHandle hMQ,
                 const char * module,
              const char * service, 
              json_object * paramObj,
              json_object * resultObj,
              int waitseconds,
              char * request_queue,
              char * reply_queue)
{
    int ret;
    json_object * msgObj;
    json_object * replyObj;

    char correlid[40];
    const char * title;
    const char * content;
    const char * errcod;
   
    if(hMQ == NULL)
        return LT_ERR_CONNECT;

    if(!module )
        return LT_ERR_PARAM;

    if(!service )
        return LT_ERR_PARAM;

    if(!paramObj)
        return LT_ERR_PARAM;

 
    lt_GenUUID(correlid, 40);

    msgObj = ltmq_msg_new(correlid, module, waitseconds, service, jo_to_json_string(paramObj));
   
    ret = ltmq_put(hMQ, request_queue, msgObj);
    if(ret == 0 && waitseconds > 0)
    {
        ret = ltmq_get(hMQ, reply_queue, correlid, NULL, msgObj, waitseconds);
        if(ret == LT_ERR_RETRY)
        {
            ret = ltmq_get(hMQ, reply_queue, correlid, NULL, msgObj, 0);
        }

        if(ret == 0)
        {
            
            title = joo_get_string(msgObj, "title");
            content = joo_get_string(msgObj, "content");
           
            errcod = title;
            if(strcmp(errcod, "SUCCESS") == 0)
            {
                if(resultObj)
                {
                    replyObj = json_tokener_parse(content);
                    {
                        json_object_object_foreach(replyObj, key, val)
                        {
                            joo_add(resultObj, key, jo_get(val));
                        }
                    }
                    jo_put(replyObj);
                }
            }
            else if(strcmp(errcod, "ERR_NOENT"))
            {
                ret = LT_ERR_NOENT;
            }
            else
            {
                ret = -1;
            }
            
        }
    }

    jo_put(msgObj);

    return ret;
}

/*
int ltas_manager_call(LTMQHandle hMQ,
              const char * service, 
              const char * param,
              char ** result,
              int waitseconds)
{
    return ltns_call_ex(hMQ,service, param, result, waitseconds, "LTAS_SYS_REQUEST","LTAS_SYS_REPLY");
}
*/

int ltns_call(LTMQHandle hMQ,
              const char * module,
              const char * service, 
              json_object * paramObj,
              json_object * resultObj,
              int waitseconds)
{
    return ltns_call_ex(hMQ, module, service, paramObj, resultObj, waitseconds, "LTNS_USER_REQUEST","LTNS_USER_REPLY");
}


/******************************************
 * hide interface function
 *****************************************/
/*
    ltns_start()
    request obj:
    {
       "req":"NS.START",
       "params":
       {
           "name":xxx,
       }
    }

    reply obj:
    {
        "errcod":"SUCCESS",
        "errstr":"xxx",
    }
*/
int ltns_start(LTMQHandle hConn,
               const char * module_name)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;

    const char * errcod;
    char * tmp;

    if(check_queue_name(module_name) == -1)
    {
        return LT_ERR_PARAM;
    }
    
    paramsObj = jo_new_object();
  
    tmp = strupr(STR_NEW(module_name));
    if(strcmp(tmp, "ALL") == 0)
    {
        joo_set_string(paramsObj, "module_name", "ALL");
    }
    else
    {
        joo_set_string(paramsObj, "module_name", module_name);
    }
    free(tmp);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "NS.START");
    joo_add(requestObj, "params", paramsObj);

    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
}

/*
    ltns_stop()
    request obj:
    {
       "req":"NS.START",
       "params":
       {
           "name":xxx,
       }
    }

    reply obj:
    {
        "errcod":"SUCCESS",
        "errstr":"xxx",
    }
*/
int ltns_stop(LTMQHandle hConn,
               const char * module_name)
{
    int ret;
    struct json_object * requestObj;
    struct json_object * paramsObj;
    struct json_object * replyObj;

    const char * errcod;
    const char * tmp;

    if(check_queue_name(module_name) == -1)
    {
        return LT_ERR_PARAM;
    }
    
    paramsObj = jo_new_object();
    
    tmp = strupr(STR_NEW(module_name));
    if(strcmp(tmp, "ALL") == 0)
    {
        joo_set_string(paramsObj, "module_name", "ALL");
    }
    else
    {
        joo_set_string(paramsObj, "module_name", module_name);
    }
    free((void *)tmp);

    joo_set_string(paramsObj, "module_name", module_name);

    requestObj = jo_new_object();
    joo_set_string(requestObj, "req", "NS.STOP");
    joo_add(requestObj, "params", paramsObj);

    ret = LT_Client_Invoke((LT_Client *)hConn, requestObj, &replyObj, -1);
    if(ret == 0)
    {
        errcod = joo_get_string(replyObj, "errcod");
        ret = errcod2int(errcod);
        jo_put(replyObj);
    }

    jo_put(requestObj);

    return ret;
}
