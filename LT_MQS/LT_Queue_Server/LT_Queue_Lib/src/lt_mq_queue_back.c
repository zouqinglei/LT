/*
 *	lt_mq_queue.c
    write by zouql 20140728
    zouqinglei@163.com
 */
#include "lt_mq_queue.h"
#include "lt_mq_packet.h"


extern char g_dbfile[MAX_PATH];

struct stLTMQManager
{
    sqlite3 * pFileDB;
    sqlite3 * pMemDB;

    struct json_object * pQueueInfo;  /* {"QUEUE1": struct LTQueueInfo,...} */

    struct json_object * pMethodsObj;

    pthread_mutex_t mutex;
}LTMQManager;

/*
    static function
*/

static int init_filedb()
{
    int result;
    char sql[1024];

    result = sqlite3_open(g_dbfile,&LTMQManager.pFileDB);
    if(result != SQLITE_OK)
        return LT_ERR_EXIST;

    sprintf(sql, "CREATE \
                    TABLE SYS_QUEUE_INFO(NAME VARCHAR(40) PRIMARY KEY NOT NULL, \
                    TYPE VARCHAR(10) NOT NULL DEFAULT 'LOCAL', \
                    PERSISTENT CHAR(1) NOT NULL DEFAULT 'N', \
					MAX_MSGNUM INT NOT NULL DEFAULT 99999, \
					MAX_MSGSIZE INT NOT NULL DEFAULT 4194304, \
                    CREATE_TIME TIMESTAMP NOT NULL DEFAULT (datetime('now','localtime')), \
                    ENABLE CHAR(1) NOT NULL DEFAULT 'Y', \
                    REMARK VARCHAR(255))");

    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);
    if(result != SQLITE_OK)
    {
        printf("create table sys_queue_info error(%d).\n",result);
    }

    return 0;
}



static int ltmq_get_size(const char * pszPersistent, const char * pszName)
{
    int result;
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    char sql[1024];
    int count;

    if(pszName == NULL)
        return 0;

    count = 0;

    if(strcmp(pszPersistent,"Y") == 0)
        pDB = LTMQManager.pFileDB;
    else
        pDB = LTMQManager.pMemDB;

    sprintf(sql, "SELECT COUNT(*) FROM %s", pszName);
    result = sqlite3_prepare(pDB, sql, -1 ,&stmt, NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        if(result == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    return count;

}

static int ltmq_queue_info_add(const char * pszName, 
                               const char * pszType,
                               const char * pszPersistent,
                               int max_msgnum,
                               int max_msgsize,
                               const char * pszEnable)
{

    LTQueueInfo * pInfo;
    char * tmp;
    
    pInfo = (LTQueueInfo *)malloc(sizeof(LTQueueInfo));
    BZERO(pInfo,sizeof(LTQueueInfo));

    pInfo->pszName = STR_NEW(pszName);
    pInfo->pszType = STR_NEW(pszType);
    pInfo->pszPersistent = STR_NEW(pszPersistent);
    pInfo->max_msgnum = max_msgnum;
    pInfo->max_msgsize = max_msgsize;  
    pInfo->pszEnable = STR_NEW(pszEnable);
  
    pInfo->current_msgnum = ltmq_get_size(pszPersistent, pszName);
   
  
    tmp = strupr(STR_NEW(pszName));
    joo_set_int(LTMQManager.pQueueInfo, tmp, (int)pInfo);
    free(tmp);

    return 0;
}

static int ltmq_queue_info_del(const char * pszName)
{
    char * tmp;
    LTQueueInfo * pInfo;

    tmp = strupr(STR_NEW(pszName));
    pInfo = (LTQueueInfo *)joo_get_int(LTMQManager.pQueueInfo, tmp);
    if(pInfo)
    {
        free(pInfo->pszName);
        free(pInfo->pszType);
        free(pInfo->pszPersistent);
        free(pInfo->pszEnable);
        free(pInfo);

        joo_del(LTMQManager.pQueueInfo, tmp);
    }
    free(tmp);

    return 0;

}


static LTQueueInfo * ltmq_queue_info_get(const char * pszName)
{
    char * tmp;
    LTQueueInfo * pInfo;

    tmp = strupr(STR_NEW(pszName));
    pInfo =  (LTQueueInfo *)joo_get_int(LTMQManager.pQueueInfo, tmp);
    free(tmp);

    return pInfo;
}

static int init_info()
{
    int result;
    char sql[1024];
    sqlite3_stmt * stmt;

    
    LTMQManager.pQueueInfo = jo_new_object();

    sprintf(sql, "SELECT NAME,TYPE,PERSISTENT,MAX_MSGNUM,MAX_MSGSIZE, \
                    ENABLE \
		          FROM SYS_QUEUE_INFO ORDER BY NAME");

    result = sqlite3_prepare(LTMQManager.pFileDB, sql, -1, &stmt, NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        while(result == SQLITE_ROW)
        {
            ltmq_queue_info_add(sqlite3_column_text(stmt,0),
                                sqlite3_column_text(stmt,1),
                                sqlite3_column_text(stmt,2),
                                sqlite3_column_int(stmt,3),
                                sqlite3_column_int(stmt,4),
                                sqlite3_column_text(stmt,5));
    
            result = sqlite3_step(stmt);
        }
    }

    sqlite3_finalize(stmt);

    return 0;
}

static destroy_info()
{
    LTQueueInfo * pInfo;

    {
        json_object_object_foreach(LTMQManager.pQueueInfo,key,val)
        {
            pInfo = (LTQueueInfo *)jo_get_int(val);
            if(pInfo)
            {
                free(pInfo->pszName);
                free(pInfo->pszType);
                free(pInfo->pszPersistent);
                free(pInfo->pszEnable);
                free(pInfo);
            }

        }

    }

    jo_put(LTMQManager.pQueueInfo);
}


static int ltmq_queue_register(const char * pszName,
                             const char * pszType,
                             const char * pszPersistent,
                             int max_msgnum,
                             int max_msgsize,
                             const char * pszEnable,
                             const char * pszRemark)
{
    int result;
    char sql[1024];

    sprintf(sql, "INSERT INTO SYS_QUEUE_INFO(NAME, TYPE, PERSISTENT,MAX_MSGNUM, MAX_MSGSIZE, \
                 ENABLE, REMARK) VALUES('%s','%s','%s',%d,%d,'%s','%s')",
                    pszName,
                    pszType,
                    pszPersistent,
                    max_msgnum,
                    max_msgsize,
                    pszEnable,
                    pszRemark);

    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);
    return result;
}

static int ltmq_queue_unregister(const char * pszName)
{
    int result;
    char sql[1024];

    sprintf(sql, "DELETE FROM SYS_QUEUE_INFO WHERE NAME = '%s'", pszName);

    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);

    return result;
}

/*
headobj struct

control
{
    protocol
    flag        / reserved,active,encrypt,compress /
    reqid
    reqname
    datalen
    oridatalen
}


in_param
{
    MSGID 
    CORRELID 
    EXPIRY 
    PRIORITY 
    USERTAG 
    CREATE_TIME 
    FLAG
    DATASIZE 
    ORIDATASIZE
    DATA BLOB
}
*/



static int ltmq_queue_create(char * pszPersistent, const char * pszName)
{
    int result;
    char sql[1024];
    sqlite3 * pDB;

    if(strcmp(pszPersistent,"Y") == 0)
        pDB = LTMQManager.pFileDB;
    else
        pDB = LTMQManager.pMemDB;

    sprintf(sql,"CREATE TABLE %s ( \
                MSGID VARCHAR(40), \
				CORRELID VARCHAR(40), \
				EXPIRY INT NOT NULL DEFAULT 0, \
				PRIORITY NOT NULL DEFAULT 0, \
                USERTAG VARCHAR(254), \
				CREATE_TIME TIMESTAMP NOT NULL DEFAULT (datetime('now','localtime')), \
                FLAG  INT NOT NULL DEFAULT 0, \
				BLOBLEN  INT NOT NULL DEFAULT 0, \
				ORIBLOBLEN  INT NOT NULL DEFAULT 0, \
                BLOB_D BLOB)",pszName);

    result = sqlite3_exec(pDB, sql, NULL, NULL, NULL);
    
    return result;
}

static int ltmq_queue_delete(char * pszPersistent, const char * pszName)
{
    int result;
    char sql[1024];
    sqlite3 * pDB;

    if(strcmp(pszPersistent,"Y") == 0)
        pDB = LTMQManager.pFileDB;
    else
        pDB = LTMQManager.pMemDB;

    sprintf(sql, "DROP TABLE '%s'", pszName);      
    result = sqlite3_exec(pDB, sql, NULL, NULL, NULL);
   
    return result;
}

static int init_memdb()
{
    int result;
    LTQueueInfo * pInfo;

    result = sqlite3_open(":memory:",&LTMQManager.pMemDB);
    if(result != SQLITE_OK)
        return LT_ERR_EXIST;

    /* create memory queue table */
    {
        json_object_object_foreach(LTMQManager.pQueueInfo,key,val)
        {
            pInfo = (LTQueueInfo *)jo_get_int(val);
            if(strcmp(pInfo->pszPersistent, "Y") != 0)
            {
                ltmq_queue_create("N",pInfo->pszName);
            }

        }

    }
    return 0;
}

/*
    ltmq_create_queue()
    request :
    headobj
    {
      "protocol":"1.0",
      "flag":0,      / reserved,active,encrypt,compress /
      "reqid":XXX,
      "reqname":'CREATE_QUEUE',
      "bloblen":0,
      "oribloblen":0,
      "in_param":{
                    "name":"queue name",
                    "type":"LOCAL" or "TOPIC" or "FORWARD",
                    "persistent":"Y" or "N",
                    "max_msgnum":0 default 99999,
                    "max_msgsize":0 default 4M,
                    "remark":""
                 }
      
     }

     reply:
     headobj
     {
        "requestid":xxx,
        "type":'CREATE_QUEUE',
        "out_param":{
            "errcod":0, or other errcod in lt_const.h
            "errstr":"The queue created success."
            } 
     }
*/

static void svc_ltmq_create_queue(LT_ClientInfo * pInfo)
{
    const char * pszName;
    const char * pszType;
    const char * pszPersistent;
    int max_msgnum;
    int max_msgsize;
    const char * pszRemark;

    LTQueueInfo * pQueueInfo;
    struct json_object * in_param_obj;
    struct json_object * out_param_obj;

    in_param_obj = joo_get(((LTMQ_Data *)pInfo->requestdata)->headobj, "in_param");
    out_param_obj = joo_get(((LTMQ_Data *)pInfo->replydata)->headobj, "out_param");
    
    pszName = joo_get_string(in_param_obj, "name");
    pszType = joo_get_string(in_param_obj, "type");
    pszPersistent = joo_get_string(in_param_obj, "persistent");
    max_msgnum = joo_get_int(in_param_obj, "max_msgnum");
    max_msgsize = joo_get_int(in_param_obj, "max_msgsize");
    pszRemark = joo_get_string(in_param_obj, "remark");

    if(max_msgnum <= 0)
    {
        max_msgnum = 999999;
    }

    if(max_msgsize <= 0)
    {
        max_msgsize = 4 * 1024 * 1024;
    }

    /* param check */
    if(strlen(pszName) == 0)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_PARAM);
        joo_set_string(out_param_obj, "errstr", "The queue name is null.");        
    }
    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(pszName);
    if(pQueueInfo)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_EXIST);
        joo_set_string(out_param_obj, "errstr", "The queue has exist."); 
    
        return;
    }
    

    ltmq_queue_register(pszName, pszType, pszPersistent, max_msgnum, max_msgsize, "Y", pszRemark);
    ltmq_queue_create(pszPersistent, pszName);   
    ltmq_queue_info_add(pszName, pszType, pszPersistent, max_msgnum, max_msgsize, "Y");
       
    joo_set_int(out_param_obj, "errcod",0);
    joo_set_string(out_param_obj, "errstr", "The queue create successed.");

    /*
    else
    {
        joo_set_int(out_paramObj, "errcod",LT_ERR_FAIL);
        joo_set_string(out_paramObj, "errstr", "The queue create failured.");
    }
    */

    return;
}


/*
    ltmq_delete_queue()
    request :
    headobj
    {
      "requestid":xxx,
      "type":'DELETE_QUEUE',
      "in_param":{
                    "name":"queue name",
                 }
      
     }

     reply:
     headobj
     {
        "requestid":xxx,
        "type":'DELETE_QUEUE',
        "out_param":{
            "errcod":0, or other errcod in lt_const.h
            "errstr":"The queue delete success."
            } 
     }
*/

static void svc_ltmq_delete_queue(LT_ClientInfo * pInfo)
{
    const char * pszName;

    struct json_object * in_param_obj;
    struct json_object * out_param_obj;
    LTQueueInfo * pQueueInfo;

    in_param_obj = joo_get(((LTMQ_Data *)pInfo->requestdata)->headobj, "in_param");
    out_param_obj = joo_get(((LTMQ_Data *)pInfo->replydata)->headobj, "out_param");
    
    pszName = joo_get_string(in_param_obj, "name");

    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(pszName);
    if(pQueueInfo == NULL)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_NOENT);
        joo_set_string(out_param_obj, "errstr", "The queue is not exist.");

        return;
    }

    ltmq_queue_delete(pQueueInfo->pszPersistent, pQueueInfo->pszName);
    ltmq_queue_unregister(pQueueInfo->pszName);
    ltmq_queue_info_del(pQueueInfo->pszName);

    joo_set_int(out_param_obj, "errcod",0);
    joo_set_string(out_param_obj, "errstr", "The queue delete successed.");

    /*
    else
    {
        joo_set_int(out_paramObj, "errcod",LT_ERR_FAIL);
        joo_set_string(out_paramObj, "errstr", "The queue delete failured.");
    }
    */

    return;
}

static void svc_ltmq_update_queue(LT_ClientInfo * pInfo)
{
}

/*
    MSGID VARCHAR(40), \
    CORRELID VARCHAR(40), \
    EXPIRY INT NOT NULL DEFAULT 0, \
    PRIORITY NOT NULL DEFAULT 0, \
    CREATE_TIME INT NULL, \
    USERTAG VARCHAR(254), \
    DATASIZE  INT NOT NULL DEFAULT 0, \
    DATA BLOB)


    ltmq_put_message()
    request :
    headobj
    {
      "requestid":xxx,
      "type":'PUT_MESSAGE',
      "in_param":{
                    "name":"queue name",
                    
                    "correlid":"uuid" or '',
                    "expiry": seconds live,
                    "priority": 0,
                    "usertag": "",
                 }   
     }

     reply:
     headobj
     {
        "requestid":xxx,
        "type":'PUT_MESSAGE',
        "out_param":{
            "errcod":0, or other errcod in lt_const.h
            "errstr":"The queue delete success."
            "correlid":'uuid'
            } 
     }


     first, before put, check the queue's info , max_msgnum, max_msgsize, enable.
     
     then put the message to the queue
     
     last notify the get wait event
*/

static int ltmq_db_put_message(const char * pszPersistent, 
                               const char * pszName,
                               const char * uuid, 
                               struct json_object * in_param_obj,
                               const void * blob)
{
    int ret;
    int result;
    char sql[1024];
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    const char * pszCorrelID;
    int bloblen;

    if(strcmp("Y", pszPersistent) == 0)
    {
        pDB = LTMQManager.pFileDB;
    }
    else
    {
        pDB = LTMQManager.pMemDB;
    }

    ret = -1;    
    
    pszCorrelID = joo_get_string(in_param_obj, "correlid");
    bloblen = joo_get_int(in_param_obj, "bloblen");
    sprintf(sql, "INSERT INTO %s(MSGID, CORRELID, EXPIRY, PRIORITY, USERTAG, FLAG, BLOBLEN, ORIBLOBLEN, BLOB_D) \
                 VALUES('%s','%s', %d, %d, '%s', %d, %d, %d, ?)", 
                 pszName,
                 uuid,
                 (strlen(pszCorrelID) == 0)?uuid:pszCorrelID,
                 joo_get_int(in_param_obj, "expiry"),
                 joo_get_int(in_param_obj, "priority"),
                 joo_get_string(in_param_obj, "usertag"),
                 joo_get_int(in_param_obj, "flag"),
                 bloblen,
                 joo_get_int(in_param_obj, "oribloblen"));

    result = sqlite3_prepare(pDB, sql, -1,&stmt,NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_bind_blob(stmt, 1, blob, bloblen, NULL);
        result = sqlite3_step(stmt);
        if(result == SQLITE_DONE)
        {
            ret = 0;     
        }
    }

    sqlite3_finalize(stmt);

    return ret;

}

static void svc_ltmq_put_message(LT_ClientInfo * pInfo)
{
    int ret;
    int result;
    char uuid[40];
    const char * pszName;
    const char * pszCorrelID;
    int bloblen;

    LTMQ_Data * pRequestData;
    LTMQ_Data * pReplyData;

    struct json_object * in_param_obj;
    struct json_object * out_param_obj;
    LTQueueInfo * pQueueInfo;

    pRequestData = (LTMQ_Data *)pInfo->requestdata;
    pReplyData = (LTMQ_Data *)pInfo->replydata;

    in_param_obj = joo_get(pRequestData->headobj, "in_param");
    out_param_obj = joo_get(pReplyData->headobj, "out_param");
    
    pszName = joo_get_string(in_param_obj, "name");
  
    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(pszName);
    if(pQueueInfo == NULL)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_NOENT);
        joo_set_string(out_param_obj, "errstr", "The queue is not exist.");

        return;
    }

    /* check max_msgnum, max_msgsize, enable*/
    if(pQueueInfo->current_msgnum >= pQueueInfo->max_msgnum)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_TOMUCH);
        joo_set_string(out_param_obj, "errstr", "The queue messages number is too much.");
        return;
    }
    bloblen = joo_get_int(in_param_obj, "bloblen");
    if(bloblen > pQueueInfo->max_msgsize)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_TOMUCH);
        joo_set_string(out_param_obj, "errstr", "The queue message size is too long.");
        return;
    }

    if(strcmp(pQueueInfo->pszEnable, "Y") != 0)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_DENIED);
        joo_set_string(out_param_obj, "errstr", "The queue is not enabled.");
        return;
    }

    lt_GenUUID(uuid, 40);
    ret = ltmq_db_put_message(pQueueInfo->pszPersistent,
                              pszName,
                              uuid,
                              in_param_obj,
                              pRequestData->blob);

    

    if(ret == 0)
    {
        joo_set_int(out_param_obj, "errcod",0);
        joo_set_string(out_param_obj, "errstr", "The put message successed.");
        pszCorrelID = joo_get_string(in_param_obj, "correlid");
        if(strlen(pszCorrelID) == 0)
        {
            joo_set_string(out_param_obj, "correlid", uuid);
        }
        else
        {
            joo_set_string(out_param_obj, "correlid",pszCorrelID);
        }        
    }
    else
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_FAIL);
        joo_set_string(out_param_obj, "errstr", "The put message failured.");
    }

    /* notify wait get */


    return;
}


static int ltmq_db_get_message(const char * pszPersistent,
                               const char * pszName,
                               struct json_object * in_param_obj, 
                               struct json_object * out_param_obj, 
                               void ** blob)
{
    int success;
    int result;
    char sql[1024];
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    int flag;
    int bloblen;
    int oribloblen;

    int rowid;

    success = -1;

    if(strcmp("Y", pszPersistent) == 0)
    {
        pDB = LTMQManager.pFileDB;
    }
    else
    {
        pDB = LTMQManager.pMemDB;
    }

    sprintf(sql, "SELECT ROWID, MSGID, CORRELID, EXPIRY, PRIORITY, \
                    CREATE_TIME, USERTAG, DATASIZE, DATA \
                  FROM %s WHERE %s LIMIT 1", 
                 pszName
                 );

    result = sqlite3_prepare(pDB, sql, -1,&stmt,NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        if(result == SQLITE_ROW)
        {
            success = 0;
            rowid = sqlite3_column_int(stmt, 0);
            joo_set_string(out_param_obj, "msgid", sqlite3_column_text(stmt, 1));
            joo_set_string(out_param_obj, "correlid", sqlite3_column_text(stmt, 2));
            joo_set_int(out_param_obj, "expiry", sqlite3_column_int(stmt, 3));
            joo_set_int(out_param_obj, "priority", sqlite3_column_int(stmt, 4));
            joo_set_string(out_param_obj, "create_time", sqlite3_column_text(stmt, 5));
            joo_set_string(out_param_obj, "usertag", sqlite3_column_text(stmt, 6));
            flag = sqlite3_column_int(stmt, 7);
            bloblen = sqlite3_column_int(stmt, 8);
            oribloblen = sqlite3_column_int(stmt, 9);

            *blob = malloc(bloblen);
            BCOPY(sqlite3_column_blob(stmt, 10), *blob, bloblen);
        
        }
    }

    sqlite3_finalize(stmt);

    if(success == 0)
    {
        sprintf(sql,"DELETE FROM %s WHERE ROWID = %d",pszName,rowid);
        result = sqlite3_exec(pDB,sql,NULL,NULL,NULL);
    }

    return success;
}


/*
    MSGID VARCHAR(40), \
    CORRELID VARCHAR(40), \
    EXPIRY INT NOT NULL DEFAULT 0, \
    PRIORITY NOT NULL DEFAULT 0, \
    CREATE_TIME INT NULL, \
    USERTAG VARCHAR(254), \
    DATASIZE  INT NOT NULL DEFAULT 0, \
    DATA BLOB)


    ltmq_get_message()
    request :
    headobj
    {
      "requestid":xxx,
      "type":'GET_MESSAGE',
      "in_param":{
                    "name":"queue name",
                    "waitseconds": 30,
             
                    --match filter
                    "msgid":"uuid" or '',
                    "correlid":"uuid" or '',
                    "priority": -1,
                    "usertag": "",
                 }   
     }

     reply:
     headobj
     {
        "requestid":xxx,
        "type":'GET_MESSAGE',
        "out_param":{
            "errcod":0, or other errcod in lt_const.h
            "errstr":"The queue delete success."

            "name":"queue name",
            "msgid":"uuid",
            "correlid":"uuid" or '',
            "expiry": seconds live,
            "priority": 0,
            "usertag": "",
            } 
     }

     if getwait is "N":
        if have message then 
            return msg
        else
            return LT_ERR_EXIST

    else
        if have message then
            return msg
        else
            add wait event in queue by clientid, wait put event.


*/

static void svc_ltmq_get_message(LT_ClientInfo * pInfo)
{
    int ret;
    const char * pszName;
    const char * pszCorrelID;
    int nExpiry;
    int nPriority;
    const char * pszUsertag; 

    LTMQ_Data * pRequestData;
    LTMQ_Data * pReplyData;

    struct json_object * in_param_obj;
    struct json_object * out_param_obj;
    LTQueueInfo * pQueueInfo;

    pRequestData = (LTMQ_Data *)pInfo->requestdata;
    pReplyData = (LTMQ_Data *)pInfo->replydata;

    in_param_obj = joo_get(pRequestData->headobj, "in_param");
    out_param_obj = joo_get(pReplyData->headobj, "out_param");


    pszName = joo_get_string(in_param_obj, "name");
    pszCorrelID = joo_get_string(in_param_obj, "correlid");
    nExpiry = joo_get_int(in_param_obj, "expiry");
    nPriority = joo_get_int(in_param_obj, "priority");
    pszUsertag = joo_get_string(in_param_obj, "usertag");
   

    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(pszName);
    if(pQueueInfo == NULL)
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_NOENT);
        joo_set_string(out_param_obj, "errstr", "The queue is not exist.");

        return;
    }

    ret = ltmq_db_get_message(pQueueInfo->pszPersistent,
        pQueueInfo->pszName,
        in_param_obj,
        out_param_obj,
        &pReplyData->blob);
        
    if(ret == 0)
    {
        joo_set_int(out_param_obj, "errcod",0);
        joo_set_string(out_param_obj, "errstr", "");
    }
    else
    {
        joo_set_int(out_param_obj, "errcod",LT_ERR_FAIL);
        joo_set_string(out_param_obj, "errstr", "The put message failured.");

        /* add get wait in the queue waitlist */
        pInfo->dotreply = 1;

    }

    return;
}

static void svc_ltmq_browse_message(LT_ClientInfo * pInfo)
{
}


static int init_other()
{
    
    LTMQManager.pMethodsObj = jo_new_object();

    joo_set_int(LTMQManager.pMethodsObj, "CREATE_QUEUE", (int)svc_ltmq_create_queue);
    joo_set_int(LTMQManager.pMethodsObj, "DELETE_QUEUE", (int)svc_ltmq_delete_queue);
    joo_set_int(LTMQManager.pMethodsObj, "UPDATE_QUEUE", (int)svc_ltmq_update_queue);
    joo_set_int(LTMQManager.pMethodsObj, "PUT_MESSAGE",  (int)svc_ltmq_put_message);
    joo_set_int(LTMQManager.pMethodsObj, "GET_MESSAGE",  (int)svc_ltmq_get_message);
    joo_set_int(LTMQManager.pMethodsObj, "BROWSE_MESSAGE", (int)svc_ltmq_browse_message);
    //joo_set_int(LTMQManager.pMethodsObj, "CLEAR_QUEUE", (int)svc_ltmq_browse_message);

    return 0;
}



/************************************************************************/
/* main process                                                         */
/************************************************************************/
/*
    1. open filedb
    2. create pQueueInfo
    3. open memdb
    4. create table in memdb by queueInfo
    5. create monitor thread
*/
int ltmq_queue_init()
{
    int ret;

    pthread_mutex_init(&LTMQManager.mutex, NULL);

    ret = init_filedb();
    if(ret != 0)
        return ret;

    ret = init_info();
    if(ret != 0)
        return ret;

    ret = init_memdb();
    if(ret != 0)
        return ret;

    ret = init_other();
    if(ret != 0)
        return ret;

    return 0;
}

/*
    1. destroy monitor thread
    2. destroy pQueueInfo
    3. close memdb
    4. close filedb
*/
int ltmq_queue_destroy()
{
    jo_put(LTMQManager.pMethodsObj);

    destroy_info();

    sqlite3_close(LTMQManager.pMemDB);
    sqlite3_close(LTMQManager.pFileDB);

    pthread_mutex_destroy(&LTMQManager.mutex);

    return 0;
}

/*
headobj
    {
       protocol
       flag        / reserved,active,encrypt,compress /
       reqid
       reqname GET_MESSAGE
       datalen
       oridatalen

      "in_param":{
                    "name":"queue name",
                    "waitseconds": 30,
             
                    --match filter
                    "msgid":"uuid" or '',
                    "correlid":"uuid" or '',
                    "priority": -1,
                    "usertag": "",
                 }   
     }

     reply:
     headobj
     {
        
        "type":'GET_MESSAGE',
        "out_param":{
            "errcod":0, or other errcod in lt_const.h
            "errstr":"The queue delete success."

            "name":"queue name",
            "msgid":"uuid",
            "correlid":"uuid" or '',
            "expiry": seconds live,
            "priority": 0,
            "usertag": "",
            } 
     }
*/

int ltmq_queue_process(LT_ClientInfo * pInfo)
{
    LTMQ_Data * pRequestData;
    LTMQ_Data * pReplyData;
    struct json_object * out_param_obj;
    const char * reqName;
    server_process_func func;

    pRequestData = (LTMQ_Data *)pInfo->requestdata;
    if(pRequestData == NULL)
        return 0;

    reqName = joo_get_string(pRequestData->headobj,"reqname");

    pReplyData = ltmq_data_new();
    pInfo->replydata = (void *)pReplyData;

    joo_set_string(pReplyData->headobj, "protocol",   joo_get_string(pRequestData->headobj,"protocol"));
    joo_set_string(pReplyData->headobj, "flag",       joo_get_string(pRequestData->headobj,"flag"));
    joo_set_string(pReplyData->headobj, "reqid",      joo_get_string(pRequestData->headobj,"reqid"));
    joo_set_string(pReplyData->headobj, "reqname",    joo_get_string(pRequestData->headobj,"reqname"));
    joo_set_string(pReplyData->headobj, "bloblen",    joo_get_string(pRequestData->headobj,"reqid"));
    joo_set_string(pReplyData->headobj, "oribloblen", joo_get_string(pRequestData->headobj,"reqid"));

    out_param_obj = jo_new_object();
    joo_add(pReplyData->headobj, "out_param", out_param_obj);

    pthread_mutex_lock(&LTMQManager.mutex);
    func = (server_process_func)joo_get_int(LTMQManager.pMethodsObj, reqName);
    if(func)
    {
        (*func)(pInfo);
    }
    else
    {
        joo_set_int(out_param_obj, "errcod", LT_ERR_NOENT);
        joo_set_string(out_param_obj, "errstr", "This request name is not exist.");
    }
    pthread_mutex_unlock(&LTMQManager.mutex);

    return 0;
}
