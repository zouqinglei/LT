/*
 *	lt_mq_queue.c
    write by zouql 20140728
    zouqinglei@163.com
 */
#include "lt_mq_queue.h"
#include "lt_mq_server_packet.h"
#include "lt_mq_server.h"
#include "lt_c_foundation.h"

#include "lt_methods.h"

extern char g_dbfile[MAX_PATH];

struct stLTMQManager
{
    sqlite3 * pFileDB;
    sqlite3 * pMemDB;

    struct json_object * pQueueInfo;  /* {"QUEUE1": struct LTQueueInfo,...} */
    pthread_mutex_t mutex;
    LT_Thread * pTimerThread;
}LTMQManager;

/*
    check_queue_name(const char * queue_name)

    if ok then return 0,
    else return -1
*/
static int check_queue_name(const char * queue_name)
{
    int i;
    int len;

    return 0;

    if(queue_name == NULL)
        return -1;

    if(strlen(queue_name) == 0)
        return -1;

    if(isalpha(queue_name[0]) == 0)
        return -1;

    len = strlen(queue_name);
    for(i = 1; i < len; i++)
    {
        if(isalnum(queue_name[i]) == 0 && queue_name[i] == '_')
        {
            return -1;
        }
    }

    return 0;
}

/*
    static function
*/

static int ltmq_info_expiry_process(LTQueueInfo * pInfo)
{
    int result;
    char sql[1024];
    sqlite3 * pDB;

    if(strcmp("Y", pInfo->pszPersistent) == 0)
    {
        pDB = LTMQManager.pFileDB;
    }
    else
    {
        pDB = LTMQManager.pMemDB;
    }
    
    sprintf(sql, "UPDATE %s SET EXPIRY = EXPIRY - 1 WHERE EXPIRY > 0 ", pInfo->pszName);
    result = sqlite3_exec(pDB,sql,NULL,NULL,NULL);
   
    sprintf(sql,"DELETE FROM %s WHERE EXPIRY = 0 ",pInfo->pszName);
    result = sqlite3_exec(pDB,sql,NULL,NULL,NULL);    
    
    return 0;
}

static void waitListObjCheck(struct json_object *listObj)
{
    int i;
    int waitseconds;
    int length;
    struct json_object * waitObj;
    length = joa_length(listObj);

    for(i = length - 1; i >= 0; i--)
    {
        waitObj = joa_get_idx(listObj,i);
        waitseconds = joo_get_int(waitObj, "waitseconds");
        waitseconds--;
        if(waitseconds <= 0)
        {
            joa_del_idx(listObj, i);
        }
        else
        {
            joo_set_int(waitObj, "waitseconds", waitseconds);
        }
    }

}


static void waitObjCheck(struct json_object * waitObj)
{
    int i;
    int length;
    struct json_object * delobjlist;

    delobjlist = jo_new_array();
    {
        json_object_object_foreach(waitObj,key,listObj)
        {
            waitListObjCheck(listObj);
            if(joa_length(listObj) == 0)
            {
                joa_add(delobjlist,jo_new_string(key));
            }

        }
        
        length = joa_length(delobjlist);
        for(i = 0; i < length; i++)
        {
            joo_del(waitObj, jo_get_string(joa_get_idx(delobjlist, i)));
        }
    }

    jo_put(delobjlist);
}

static int ltmq_info_waitobj_process(LTQueueInfo * pInfo)
{
    waitObjCheck(pInfo->waitCorrelidObj);
    waitObjCheck(pInfo->waitTagObj);
    waitListObjCheck(pInfo->waitNoneListObj);


    return 0;
}


/*
    handle_timer.
    1. waitobj
    2. queue's expiry
*/
pthread_handler_decl(handle_timer,arg)
{
    LT_Thread * pThread;
	LTQueueInfo * pInfo;

    pThread = LTMQManager.pTimerThread;

	while(!pThread->abort_loop)
    {
        SLEEP(1);
         
        pthread_mutex_lock(&LTMQManager.mutex);
        {
            json_object_object_foreach(LTMQManager.pQueueInfo, key, val)
            {
                pInfo = (LTQueueInfo *)jo_get_int(val);
                ltmq_info_waitobj_process(pInfo);
                ltmq_info_expiry_process(pInfo);
            }
        }
        pthread_mutex_unlock(&LTMQManager.mutex);
    }

    LT_Thread_exit(pThread);
}


static int init_filedb()
{
    int result;
    char sql[1024];

    result = sqlite3_open(g_dbfile,&LTMQManager.pFileDB);
    if(result != SQLITE_OK)
        return LT_ERR_EXIST;

    sprintf(sql, "CREATE \
                    TABLE SYS_QUEUE_INFO(NAME VARCHAR(40) PRIMARY KEY NOT NULL, \
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

    sprintf(sql, "INSERT INTO SYS_QUEUE_INFO(NAME, REMARK) VALUES('LTNS_USER_REQUEST','system queue, don''t delete it.')");
    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);
    
    sprintf(sql, "INSERT INTO SYS_QUEUE_INFO(NAME, REMARK) VALUES('LTNS_USER_REPLY','system queue, don''t delete it.')");
    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);
    
    sprintf(sql, "INSERT INTO SYS_QUEUE_INFO(NAME, REMARK) VALUES('LTNS_SYS_REQUEST','system queue, don''t delete it.')");
    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);
    
    sprintf(sql, "INSERT INTO SYS_QUEUE_INFO(NAME, REMARK) VALUES('LTNS_SYS_REPLY','system queue, don''t delete it.')");
    result = sqlite3_exec(LTMQManager.pFileDB, sql, NULL, NULL, NULL);

    return 0;
}



static int ltmq_db_get_size(const char * pszPersistent, const char * queue_name)
{
    int result;
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    char sql[1024];
    int count;

    if(queue_name == NULL)
        return 0;

    count = 0;

    if(strcmp(pszPersistent,"Y") == 0)
        pDB = LTMQManager.pFileDB;
    else
        pDB = LTMQManager.pMemDB;

    sprintf(sql, "SELECT COUNT(*) FROM %s", queue_name);
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

static int ltmq_queue_info_add(const char * queue_name, 
                               const char * pszPersistent,
                               int max_msgnum,
                               int max_msgsize,
                               const char * pszEnable)
{

    LTQueueInfo * pInfo;
    
    pInfo = (LTQueueInfo *)malloc(sizeof(LTQueueInfo));
    BZERO(pInfo,sizeof(LTQueueInfo));

    pInfo->pszName = STR_NEW(queue_name);
    pInfo->pszPersistent = STR_NEW(pszPersistent);
    pInfo->max_msgnum = max_msgnum;
    pInfo->max_msgsize = max_msgsize;  
    pInfo->pszEnable = STR_NEW(pszEnable);
  
    pInfo->current_msgnum = ltmq_db_get_size(pszPersistent, queue_name);
    
    pInfo->waitCorrelidObj = jo_new_object();
    pInfo->waitTagObj = jo_new_object();
    pInfo->waitNoneListObj = jo_new_array();
  
  
    joo_set_int(LTMQManager.pQueueInfo, queue_name, (int)pInfo);

    return 0;
}

static int ltmq_queue_info_del(const char * queue_name)
{
    LTQueueInfo * pInfo;

    pInfo = (LTQueueInfo *)joo_get_int(LTMQManager.pQueueInfo, queue_name);
    if(pInfo)
    {
        free(pInfo->pszName);
        free(pInfo->pszPersistent);
        free(pInfo->pszEnable);
        free(pInfo);

        joo_del(LTMQManager.pQueueInfo, queue_name);
    }

    return 0;

}


static LTQueueInfo * ltmq_queue_info_get(const char * queue_name)
{
    LTQueueInfo * pInfo;

    if(!queue_name)
        return NULL;

    pInfo =  (LTQueueInfo *)joo_get_int(LTMQManager.pQueueInfo, queue_name);

    return pInfo;
}

static int init_info()
{
    int result;
    char sql[1024];
    sqlite3_stmt * stmt;

    
    LTMQManager.pQueueInfo = jo_new_object();

    sprintf(sql, "SELECT UPPER(NAME),PERSISTENT,MAX_MSGNUM,MAX_MSGSIZE, \
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
                                sqlite3_column_int(stmt,2),
                                sqlite3_column_int(stmt,3),
                                sqlite3_column_text(stmt,4));
    
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
                free(pInfo->pszPersistent);
                free(pInfo->pszEnable);
                jo_put(pInfo->waitCorrelidObj);
                jo_put(pInfo->waitTagObj);
                jo_put(pInfo->waitNoneListObj);
                free(pInfo);
            }

        }

    }

    jo_put(LTMQManager.pQueueInfo);
}


static int ltmq_db_queue_register(const char * queue_name,
                             const char * pszPersistent,
                             int max_msgnum,
                             int max_msgsize,
                             const char * pszEnable,
                             const char * pszRemark)
{
    int ret;
    int result;
    char sql[1024];
    sqlite3_stmt * stmt;

    ret = -1;

    sprintf(sql, "INSERT INTO SYS_QUEUE_INFO(NAME, PERSISTENT,MAX_MSGNUM, MAX_MSGSIZE, \
                 ENABLE, REMARK) VALUES('%s','%s',%d,%d,'%s',?)",
                    queue_name,
                    pszPersistent,
                    max_msgnum,
                    max_msgsize,
                    pszEnable);

    result = sqlite3_prepare(LTMQManager.pFileDB, sql, -1, &stmt, NULL);
    if(result == SQLITE_OK)
    {
        sqlite3_bind_text(stmt,1, pszRemark, -1, NULL);
        result = sqlite3_step(stmt);
        if(result == SQLITE_DONE)
        {
            ret = 0;
        }
    }
    
    sqlite3_finalize(stmt);
 
    return ret;
}

static int ltmq_db_queue_unregister(const char * queue_name)
{
    int result;
    char sql[1024];

    sprintf(sql, "DELETE FROM SYS_QUEUE_INFO WHERE NAME = '%s'", queue_name);

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



static int ltmq_db_queue_create(const char * pszPersistent, const char * queue_name)
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
                TAG VARCHAR(40), \
				EXPIRY INT NOT NULL DEFAULT 0, \
                TITLE VARCHAR(256), \
                CONTENT VARCHAR(1024), \
                CREATE_TIME TIMESTAMP NOT NULL DEFAULT (datetime('now','localtime'))); \
                CREATE INDEX [Index%s] ON [%s] ([MSGID], [CORRELID], [TAG], [EXPIRY]);",
                queue_name,queue_name,queue_name);

    result = sqlite3_exec(pDB, sql, NULL, NULL, NULL);

    return result;
}

static int ltmq_db_queue_delete(char * pszPersistent, const char * queue_name)
{
    int result;
    char sql[1024];
    sqlite3 * pDB;

    if(strcmp(pszPersistent,"Y") == 0)
        pDB = LTMQManager.pFileDB;
    else
        pDB = LTMQManager.pMemDB;

    sprintf(sql, "DROP TABLE '%s'", queue_name);      
    result = sqlite3_exec(pDB, sql, NULL, NULL, NULL);
    
    sprintf(sql, "DROP INDEX 'Index%s'", queue_name);  
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
                ltmq_db_queue_create("N",key);
            }

        }

    }
    return 0;
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
static void ltmq_queue_create(LT_RequestInfo * pInfo)
{
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    const char * pszPersistent;
    int max_msgnum;
    int max_msgsize;
    const char * pszRemark;

    LTQueueInfo * pQueueInfo;

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name  = joo_get_string(paramsObj, "queue_name");
    pszPersistent = joo_get_string(paramsObj, "persistent");
    max_msgnum  = joo_get_int(paramsObj, "max_msgnum");
    max_msgsize = joo_get_int(paramsObj, "max_msgsize");
    pszRemark   = joo_get_string(paramsObj, "remark");

    if(max_msgnum <= 0)
    {
        max_msgnum = 999999;
    }

    if(max_msgsize <= 0)
    {
        max_msgsize = 4 * 1024 * 1024;
    }

    /* check queue_name */
    if(check_queue_name(queue_name) == -1)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_PARAM");
        joo_set_string(replyDataObj, "errstr", "The queue name is invaild.");       
        return;
    }

    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_EXIST");
        joo_set_string(replyDataObj, "errstr", "The queue has exist."); 
        return;
    }

    ltmq_db_queue_register(queue_name, pszPersistent, max_msgnum, max_msgsize, "Y", pszRemark);
    ltmq_db_queue_create(pszPersistent, queue_name);   
    ltmq_queue_info_add(queue_name, pszPersistent, max_msgnum, max_msgsize, "Y");
       
    joo_set_string(replyDataObj, "errcod", "SUCCESS");
    joo_set_string(replyDataObj, "errstr", "The queue create successed.");
    
    return;
}

static void svc_ltmq_queue_create(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_queue_create(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}


/*
    ltmq_delete_queue()

    requestData object:
    {
        "req":'QUEUE.DELETE',
        "params": {
            "queue_name":"xxxx",
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":"The queue created success."
     }
*/
static void ltmq_queue_delete(LT_RequestInfo * pInfo)
{
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    LTQueueInfo * pQueueInfo;

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "queue_name");
 
    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo == NULL)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_EXIST");
        joo_set_string(replyDataObj, "errstr", "The queue is not exist.");
        return;
    }

    ltmq_db_queue_delete(pQueueInfo->pszPersistent, queue_name);
    ltmq_db_queue_unregister(queue_name);
    ltmq_queue_info_del(queue_name);

    joo_set_string(replyDataObj, "errcod", "SUCCESS");
    joo_set_string(replyDataObj, "errstr", "Delete queue success.");
    
    return;
}

static void svc_ltmq_queue_delete(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_queue_delete(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}


/*
    ltmq_queue_clear()

    requestData object:
    {
        "req":'QUEUE.DELETE',
        "params": {
            "queue_name":"xxxx",
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":"The queue created success."
     }
*/

static void ltmq_queue_clear(LT_RequestInfo * pInfo)
{
    int result;
    char sql[256];
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    LTQueueInfo * pQueueInfo;

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "queue_name");
 

    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo == NULL)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_EXIST");
        joo_set_string(replyDataObj, "errstr", "The queue is not exist.");
        return;
    }

    sprintf(sql,"DELETE FROM %s ",queue_name);

    if(strcmp("Y", pQueueInfo->pszPersistent) == 0)
    {
        result = sqlite3_exec(LTMQManager.pFileDB,sql,NULL,NULL,NULL);
    }
    else
    {
        result = sqlite3_exec(LTMQManager.pMemDB,sql,NULL,NULL,NULL);
    }

    if(result == SQLITE_OK)
    {
        joo_set_string(replyDataObj, "errcod", "SUCCESS");
        joo_set_string(replyDataObj, "errstr", "The queue clear successed.");
    }
    else
    {
        joo_set_string(replyDataObj, "errcod", "ERR_FAILURE");
        joo_set_string(replyDataObj, "errstr", "The queue clear failured.");
    }
  
    return;
}
static void svc_ltmq_queue_clear(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_queue_clear(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}

/*

    ltmq_queue_info()

    requestData object:
    {
        "req":'QUEUE.INFO',
        "params": {
            "name":"xxxx",
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":"The queue created success."
        "data": {}
     }


    recordset:
    {"column":["a","b","c","d"],
     "rows":[
             [  ],
             [  ]
            ]
    }
*/



static int ltmq_db_queue_info(const char * queue_name, json_object * hRecord)
{
    int result;
    char sql[1024];
    char sqlwhere[1024];
    sqlite3_stmt * stmt;
    int row,col;

    if(strlen(queue_name) == 0)
    {
        sqlwhere[0] = '\0';
    }
    else
    {
        sprintf(sqlwhere, " where name = '%s' ", queue_name);
    }

    sprintf(sql, "SELECT NAME,PERSISTENT,MAX_MSGNUM,MAX_MSGSIZE, \
                    CREATE_TIME, REMARK \
		          FROM SYS_QUEUE_INFO %s ORDER BY NAME", sqlwhere);

    result = sqlite3_prepare(LTMQManager.pFileDB, sql, -1, &stmt, NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        while(result == SQLITE_ROW)
        {
            row = ltr_insertrow(hRecord, -1);
            for(col = 0; col < 6; col++)
            {
                ltr_setitem(hRecord, row, col, sqlite3_column_text(stmt, col)); 
            }

            result = sqlite3_step(stmt);
        }
    }

    sqlite3_finalize(stmt);

    return 0;
}

static int ltmq_queue_get_current_msgnum(json_object * hRecord)
{
    int rowcount;
    int row;
    const char * queue_name;
    LTQueueInfo * pInfo;
    char tmp[30];

    rowcount = ltr_getrowcount(hRecord);
    for(row = 0; row < rowcount; row++)
    {
        queue_name = ltr_getitem(hRecord, row, 0);
        pInfo = ltmq_queue_info_get(queue_name);
        if(pInfo)
        {
            sprintf(tmp,"%d",pInfo->current_msgnum);
            ltr_setitem(hRecord,row,6,tmp);
        }
    }
    
    return 0;
}   


/*
SELECT NAME,TYPE,PERSISTENT,MAX_MSGNUM,MAX_MSGSIZE, \
                    ENABLE \CREATE_TIME
                    current_msgnum
*/

static void ltmq_queue_info(LT_RequestInfo * pInfo)
{
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    json_object * hRecord;


    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "name");

    hRecord = ltr_new();
    ltr_appendcol(hRecord, "NAME", 1, 1);
    ltr_appendcol(hRecord, "DB", 1, 1);
    ltr_appendcol(hRecord, "MAX_NUM", 1, 1);
    ltr_appendcol(hRecord, "MAX_SIZE", 1, 1);
    ltr_appendcol(hRecord, "CREATE_TIME", 1, 1);
    //ltr_appendcol(hRecord, "ENABLE", 1, 1);
    ltr_appendcol(hRecord, "REMARK", 1, 1);
    ltr_appendcol(hRecord, "CUR_NUM", 1, 1);
    
    /* queue info */
    ltmq_db_queue_info(queue_name, hRecord);
    ltmq_queue_get_current_msgnum(hRecord);

    joo_set_string(replyDataObj, "errcod", "SUCCESS");
    joo_set_string(replyDataObj, "errstr", "");
    
    joo_add(replyDataObj, "data", hRecord);
    

    return;
  
}

static void svc_ltmq_queue_info(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_queue_info(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}



/*
    MSGID VARCHAR(40), \
    CORRELID VARCHAR(40), \
    EXPIRY INT NOT NULL DEFAULT 0, \
    PRIORITY NOT NULL DEFAULT 0, \
    CREATE_TIME INT NULL, \
    TITLE VARCHAR(256), \
    CONTENT VARCHAR(256), \
    DATASIZE  INT NOT NULL DEFAULT 0, \
    DATA BLOB)


     first, before put, check the queue's info , max_msgnum, max_msgsize, enable.
     
     then put the message to the queue
     
     last notify the get wait event
*/

static boolean ltmq_db_message_put(const char * pszPersistent, 
                               const char * queue_name,
                               struct json_object * msgdataObj)
{
    boolean bRet;
    int result;
    char sql[1024];
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    char uuid[40];

    if(strcmp("Y", pszPersistent) == 0)
    {
        pDB = LTMQManager.pFileDB;
    }
    else
    {
        pDB = LTMQManager.pMemDB;
    }

    bRet = FALSE;    
 
    lt_GenUUID(uuid, 40);
    
    sprintf(sql, "INSERT INTO %s(MSGID, CORRELID, TAG, EXPIRY, TITLE, CONTENT) \
                 VALUES('%s',?, ?, ?, ?, ?)", 
                 queue_name,
                 uuid);

    result = sqlite3_prepare(pDB, sql, -1,&stmt,NULL);
    if(result == SQLITE_OK)
    {
        sqlite3_bind_text(stmt,1,joo_get_string(msgdataObj, "correlid"),-1,NULL);
        sqlite3_bind_text(stmt,2,joo_get_string(msgdataObj, "tag"),-1,NULL);
        sqlite3_bind_int (stmt,3,joo_get_int(msgdataObj, "expiry"));
        sqlite3_bind_text(stmt,4,joo_get_string(msgdataObj, "title"),-1,NULL);
        sqlite3_bind_text(stmt,5,joo_get_string(msgdataObj, "content"),-1,NULL);

        result = sqlite3_step(stmt);
        if(result == SQLITE_DONE)
        {
            bRet = TRUE;     
        }
    }

    sqlite3_finalize(stmt);

    return bRet;
}

static void ltmq_notify(struct json_object * waitObj)
{
    //char * notifyData;
    struct json_object * outObj;

    outObj = jo_new_object();
    joo_set_string(outObj, "errcod", "ERR_RETRY");
    joo_set_string(outObj, "errstr", "please retry get.");

    //notifyData = STR_NEW(jo_to_json_string(outObj));

    //jo_put(outObj);

    //LT_Server_Reply(ltmq_server_instance(), pInfo);
    LT_Server_Send(ltmq_server_instance(), 
        joo_get_int(waitObj, "clientID"), 
        joo_get_int(waitObj, "requestID"),
        outObj);

}

static void waitObjNotify(LTQueueInfo * pQueueInfo, struct json_object * msgdataObj)
{
    struct json_object * listObj;
    struct json_object * waitObj;
    const char * correlid;
    const char * tag;

    correlid = joo_get_string(msgdataObj, "correlid");
    tag = joo_get_string(msgdataObj, "tag");

    /* correlid */
    if(strlen(correlid) > 0)
    {
        listObj = joo_get(pQueueInfo->waitCorrelidObj, correlid);
        if(listObj != NULL )
        {
            waitObj = joa_get_idx(listObj,0);
            if(waitObj)
            {
                ltmq_notify(waitObj);
                joa_del_idx(listObj,0);
                if(joa_length(listObj) == 0)
                {
                    joo_del(pQueueInfo->waitCorrelidObj, correlid);
                    return;
                }
            }
        }
    }

    /* tag */
    if(strlen(tag) > 0)
    {
        listObj = joo_get(pQueueInfo->waitTagObj, tag);
        if(listObj != NULL )
        {
            waitObj = joa_get_idx(listObj,0);
            if(waitObj)
            {
                ltmq_notify(waitObj);
                joa_del_idx(listObj,0);
                if(joa_length(listObj) == 0)
                {
                    joo_del(pQueueInfo->waitTagObj, tag);
                    return;
                }
            }
        }
    }

    /* none */
    waitObj = joa_get_idx(pQueueInfo->waitNoneListObj,0);
    if(waitObj)
    {
        ltmq_notify(waitObj);      
        joa_del_idx(pQueueInfo->waitNoneListObj,0);
    }

    
    return;
}

/*
    ltmq_message_put()

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

static void ltmq_message_put(LT_RequestInfo * pInfo)
{
    boolean bRet;
    struct json_object * msgdataObj;

    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    LTQueueInfo * pQueueInfo;

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "queue_name");

    /* if exist? */
    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo == NULL)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_NOENT");
        joo_set_string(replyDataObj, "errstr", "The queue is not exist.");
        return;
    }

    /* check max_msgnum, max_msgsize, enable*/
    if(pQueueInfo->current_msgnum >= pQueueInfo->max_msgnum)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_TOMUCH");
        joo_set_string(replyDataObj, "errstr", "The queue messages number is too much.");
        return;
    }
    

    if(strcmp(pQueueInfo->pszEnable, "Y") != 0)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_DENIED");
        joo_set_string(replyDataObj, "errstr", "The queue is not enabled.");
        return;
    }

    msgdataObj = joo_get(paramsObj,"data");
    bRet = ltmq_db_message_put(pQueueInfo->pszPersistent, queue_name, msgdataObj);
    
    if(bRet)
    {
        pQueueInfo->current_msgnum++;
        joo_set_string(replyDataObj, "errcod", "SUCCESS");
        joo_set_string(replyDataObj, "errstr", "The put message successed.");

        /* notify wait get */
        waitObjNotify(pQueueInfo,msgdataObj);
    }
    else
    {
        joo_set_string(replyDataObj, "errcod", "ERR_FAILURE");
        joo_set_string(replyDataObj, "errstr", "The put message failured.");
    }

    return;
}

static void svc_ltmq_message_put(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_message_put(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}






static boolean ltmq_db_message_get(const char * pszPersistent,
                               const char * queue_name,
                               const char * pszCorrelID,
                               const char * pszTag,
                               struct json_object * outObj)
{
    boolean success;
    boolean bBindCorrelID;
    boolean bBindTag;
    int result;
    char sql[1024];
    char wheresql[256];
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    struct json_object * msgdataObj;
    int rowid;

    success = FALSE;
    bBindCorrelID = FALSE;
    bBindTag = FALSE;

    if(strcmp("Y", pszPersistent) == 0)
    {
        pDB = LTMQManager.pFileDB;
    }
    else
    {
        pDB = LTMQManager.pMemDB;
    }
   
    wheresql[0] = '\0';

    if(strlen(pszCorrelID) > 0)
    {
        bBindCorrelID = TRUE;
        if(strlen(pszTag) > 0)
        {
            bBindTag = TRUE;
            sprintf(wheresql, " where correlid = ? and tag = ? ");
        }
        else
        {
            sprintf(wheresql, " where correlid = ? ");
        }
    }
    else if(strlen(pszTag) > 0)
    {
        bBindTag = TRUE;
        sprintf(wheresql, " where tag = ? ");
    }

    sprintf(sql, "SELECT ROWID, MSGID, CORRELID, TAG, EXPIRY, \
                    TITLE, CONTENT, CREATE_TIME \
                  FROM %s %s LIMIT 1", 
                 queue_name,
                 wheresql);

    result = sqlite3_prepare(pDB, sql, -1,&stmt,NULL);
    if(result == SQLITE_OK)
    {
        if(bBindCorrelID)
        {
            sqlite3_bind_text(stmt, 1, pszCorrelID, -1, NULL);
            if(bBindTag)
            {
                sqlite3_bind_text(stmt, 2, pszTag, -1, NULL);
            }
        }
        else if(bBindTag)
        {
            sqlite3_bind_text(stmt, 1, pszTag, -1, NULL);
        }

        result = sqlite3_step(stmt);
        if(result == SQLITE_ROW)
        {
            success = TRUE;
            msgdataObj = jo_new_object();

            rowid = sqlite3_column_int(stmt, 0);

            joo_set_string(msgdataObj, "msgid", sqlite3_column_text(stmt, 1));
            joo_set_string(msgdataObj, "correlid", sqlite3_column_text(stmt, 2));
            joo_set_string(msgdataObj, "tag", sqlite3_column_text(stmt, 3));
            joo_set_int   (msgdataObj, "expiry", sqlite3_column_int(stmt, 4));
            joo_set_string(msgdataObj, "title", sqlite3_column_text(stmt, 5));
            joo_set_string(msgdataObj, "content", sqlite3_column_text(stmt, 6));
            joo_set_string(msgdataObj, "create_time", sqlite3_column_text(stmt, 7));

            joo_add(outObj, "data", msgdataObj); 
        }
    }

    sqlite3_finalize(stmt);

    if(success == TRUE)
    {
        sprintf(sql,"DELETE FROM %s WHERE ROWID = %d",queue_name,rowid);
        result = sqlite3_exec(pDB,sql,NULL,NULL,NULL);
    }

    return success;
}


static void waitListObjAdd(struct json_object * waitKeyObj, const char * key, struct json_object * waitObj)
{
    struct json_object * listObj;
    listObj = joo_get(waitKeyObj, key);
    if(listObj == NULL)
    {
        listObj = jo_new_array();
        joo_add(waitKeyObj, key, listObj);
    }
    
    joa_add(listObj, waitObj);
}

static void waitObjAdd(LTQueueInfo * pQueueInfo,
                       const char * correlid, 
                       const char * tag,
                       unsigned int clientID,
                       unsigned int requestID,
                       int waitseconds)
{
    struct json_object * waitObj;
    

    waitObj = jo_new_object();
    joo_set_int(waitObj,"clientID",clientID);
    joo_set_int(waitObj,"requestID", requestID);
    joo_set_int(waitObj,"waitseconds",waitseconds);

    if(strlen(correlid) > 0)
    {
        waitListObjAdd(pQueueInfo->waitCorrelidObj, correlid, waitObj);
    }
    else if(strlen(tag) > 0)
    {
        waitListObjAdd(pQueueInfo->waitTagObj, tag, waitObj);
    }
    else
    {
        joa_add(pQueueInfo->waitNoneListObj, waitObj);
    }

    

}
/*
    ltmq_message_get()

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
static void ltmq_message_get(LT_RequestInfo * pInfo)
{
    boolean bRet;
  
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    LTQueueInfo * pQueueInfo;
    const char * pszCorrelID;
    const char * pszTag;
    int waitseconds;

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "queue_name");
    pszCorrelID = joo_get_string(paramsObj, "correlid");
    pszTag = joo_get_string(paramsObj, "tag");
    waitseconds = joo_get_int(paramsObj, "waitseconds");
    /* if exist? */

    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo == NULL)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_NOENT");
        joo_set_string(replyDataObj, "errstr", "The queue is not exist.");
        return;
    }

    if(strcmp(pQueueInfo->pszEnable, "Y") != 0)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_DENIED");
        joo_set_string(replyDataObj, "errstr", "The queue is not enabled.");
        return;
    }

    bRet = ltmq_db_message_get(pQueueInfo->pszPersistent,
        queue_name,
        pszCorrelID,
        pszTag,
        replyDataObj);
     
    if(bRet)
    {
       
        pQueueInfo->current_msgnum--;
        
        joo_set_string(replyDataObj, "errcod","SUCCESS");
        joo_set_string(replyDataObj, "errstr", "");
    }
    else
    {
        joo_set_string(replyDataObj, "errcod", "ERR_NODATA");
        joo_set_string(replyDataObj, "errstr", "The get message failured.");
      
        if(waitseconds > 0)
        {
            pInfo->dontReply = 1;
            waitObjAdd(pQueueInfo, pszCorrelID,pszTag, pInfo->clientID, pInfo->requestID,waitseconds);            
        }
    }

    return;
}

static void svc_ltmq_message_get(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_message_get(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}

static int ltmq_db_message_browse(const char * pszPersistent,
                                  const char * queue_name,
                                  int start,
                                  int limit,
                                  json_object * hRecord)
{
    int result;
    char sql[1024];
    sqlite3 * pDB;
    sqlite3_stmt * stmt;
    int row,col;

    if(strcmp("Y", pszPersistent) == 0)
    {
        pDB = LTMQManager.pFileDB;
    }
    else
    {
        pDB = LTMQManager.pMemDB;
    }

    sprintf(sql, "SELECT MSGID, CORRELID, TAG, EXPIRY, \
                    TITLE, CREATE_TIME \
                  FROM %s LIMIT %d OFFSET %d", 
                 queue_name,
                 limit,
                 start);

    result = sqlite3_prepare(pDB, sql, -1,&stmt,NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        while(result == SQLITE_ROW)
        {
            row = ltr_insertrow(hRecord, -1);
            for(col = 0; col < 6; col++)
            {
                ltr_setitem(hRecord, row, col, sqlite3_column_text(stmt, col)); 
            }

            result = sqlite3_step(stmt);
        }
    }

    sqlite3_finalize(stmt);

    return 0;
}

/*

    MSGID VARCHAR(40), 
    CORRELID VARCHAR(40), 
    EXPIRY INT NOT NULL DEFAULT 0, 
    USERTAG VARCHAR(254), 
    BLOBLEN  INT NOT NULL DEFAULT 0, 
    CREATE_TIME

      ltmq_message_get()

    requestData object:
    {
        "req":'QUEUE.GETMSG',
        "params": {
            "queue_name":"xxxx",
            "start":"xxx",
            "limit":"xxx", 
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":""
        "data":{
               
               }
     }
*/
static void ltmq_message_browse(LT_RequestInfo * pInfo)
{
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    LTQueueInfo * pQueueInfo;
    json_object * hRecord;
    int start;
    int limit;


    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "queue_name");
    start = joo_get_int(paramsObj, "start");
    limit = joo_get_int(paramsObj, "limit");
    /* if exist? */

    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo == NULL)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_NOENT");
        joo_set_string(replyDataObj, "errstr", "The queue is not exist.");
        return;
    }

    hRecord = ltr_new();
    ltr_appendcol(hRecord, "MSGID", 1, 1);
    ltr_appendcol(hRecord, "CORRELID", 1, 1);
    ltr_appendcol(hRecord, "TAG", 1, 1);
    ltr_appendcol(hRecord, "EXPIRY", 1, 1);
    ltr_appendcol(hRecord, "TITLE", 1, 1);
    ltr_appendcol(hRecord, "CREATE_TIME", 1, 1);
    ltr_appendcol(hRecord, "CONTENT", 1, 1);

  
    ltmq_db_message_browse(pQueueInfo->pszPersistent, queue_name, start, limit, hRecord);

    joo_set_string(replyDataObj, "errcod", "SUCCESS");
    joo_set_string(replyDataObj, "errstr", "");
    joo_add(replyDataObj, "data", hRecord);
}

static void svc_ltmq_message_browse(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_message_browse(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}

/**************************************
 * browse msg
 *************************************/
static boolean ltmq_db_message_look(const char * pszPersistent,
                               const char * queue_name,
                               const char * msgid,
                               struct json_object * outObj)
{
    int success;
    int result;
    char sql[1024];
    sqlite3 * pDB;
    sqlite3_stmt * stmt;

    struct json_object * msgdataObj;

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

    sprintf(sql, "SELECT MSGID, CORRELID, TAG, EXPIRY, \
                    TITLE, CONTENT, CREATE_TIME \
                  FROM %s WHERE MSGID = '%s'", 
                 queue_name,
                 msgid);

    result = sqlite3_prepare(pDB, sql, -1,&stmt,NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        if(result == SQLITE_ROW)
        {
            success = 0;
            msgdataObj = jo_new_object();

            rowid = sqlite3_column_int(stmt, 0);

            joo_set_string(msgdataObj, "msgid", sqlite3_column_text(stmt, 1));
            joo_set_string(msgdataObj, "correlid", sqlite3_column_text(stmt, 2));
            joo_set_string(msgdataObj, "tag", sqlite3_column_text(stmt, 3));
            joo_set_int   (msgdataObj, "expiry", sqlite3_column_int(stmt, 4));
            joo_set_string(msgdataObj, "title", sqlite3_column_text(stmt, 5));
            joo_set_string(msgdataObj, "content", sqlite3_column_text(stmt, 6));
            joo_set_string(msgdataObj, "create_time", sqlite3_column_text(stmt, 7));

            joo_add(outObj, "data", msgdataObj);
        
        }
    }

    sqlite3_finalize(stmt);

    return success;
}

static void ltmq_message_browse_msg(LT_RequestInfo * pInfo)
{
    boolean bRet;
  
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    LTQueueInfo * pQueueInfo;
    const char * msgid;

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "queue_name");
    msgid = joo_get_string(paramsObj, "msgid");
   
    /* if exist? */

    pQueueInfo = ltmq_queue_info_get(queue_name);
    if(pQueueInfo == NULL)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_NOENT");
        joo_set_string(replyDataObj, "errstr", "The queue is not exist.");
        return;
    }

    if(strcmp(pQueueInfo->pszEnable, "Y") != 0)
    {
        joo_set_string(replyDataObj, "errcod", "ERR_DENIED");
        joo_set_string(replyDataObj, "errstr", "The queue is not enabled.");
        return;
    }

    bRet = ltmq_db_message_look(pQueueInfo->pszPersistent,
        queue_name,
        msgid,
        replyDataObj);
     
    if(bRet)
    {
        joo_set_string(replyDataObj, "errcod","SUCCESS");
        joo_set_string(replyDataObj, "errstr", "");
    }
    else
    {
        joo_set_string(replyDataObj, "errcod", "ERR_NODATA");
        joo_set_string(replyDataObj, "errstr", "This message not found.");
      
    }

    return;
}


static void svc_ltmq_message_browse_msg(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTMQManager.mutex);
    ltmq_message_browse_msg(pInfo);
    pthread_mutex_unlock(&LTMQManager.mutex);
}

/*
    init the 1 second timer, for loop check some work, such as waitobj...
*/
static int init_timer()
{
    LTMQManager.pTimerThread = LT_Thread_new(handle_timer, NULL);
    LT_Thread_start(LTMQManager.pTimerThread);

    return 0;
}

static int init_method()
{ 
    lt_methods_register( "QUEUE.CREATE", svc_ltmq_queue_create);
    lt_methods_register( "QUEUE.DELETE", svc_ltmq_queue_delete);
    lt_methods_register( "QUEUE.CLEAR", svc_ltmq_queue_clear);
    lt_methods_register( "QUEUE.INFO", svc_ltmq_queue_info);
    
    lt_methods_register( "QUEUE.PUTMSG",  svc_ltmq_message_put);
    lt_methods_register( "QUEUE.GETMSG",  svc_ltmq_message_get);
    lt_methods_register( "QUEUE.BROWSE",  svc_ltmq_message_browse);
    lt_methods_register( "QUEUE.BROWSEMSG",  svc_ltmq_message_browse_msg);
    
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

    ret = init_timer();
    if(ret != 0)
        return ret;

    ret = init_method();
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
    LT_Thread_stop(LTMQManager.pTimerThread);
    LT_Thread_free(LTMQManager.pTimerThread);

    destroy_info();

    sqlite3_close(LTMQManager.pMemDB);
    sqlite3_close(LTMQManager.pFileDB);

    pthread_mutex_destroy(&LTMQManager.mutex);

    return 0;
}


void inter_mq_put(const char * queueName, 
                         const char *correlid, 
                         const char * tag, 
                         int expiry, 
                         const char * title, 
                         const char * content)
{
    LT_RequestInfo info;
    json_object * paramsObj; 
    json_object * dataObj; 

    BZERO(&info, sizeof(LT_RequestInfo));

    info.requestData = jo_new_object();
    info.replyData = jo_new_object();

    paramsObj = jo_new_object();

    joo_set_string(paramsObj, "queue_name", queueName);

    dataObj = jo_new_object();
    joo_set_string(dataObj, "correlid", (correlid == NULL)?"":correlid);
    joo_set_string(dataObj, "tag", (tag == NULL)?"":tag);
    joo_set_int(dataObj, "expiry", expiry);
    joo_set_string(dataObj, "title", (title == NULL)?"":title);
    joo_set_string(dataObj, "content", (content == NULL)?"":content);
    

    joo_add(paramsObj, "data", dataObj);
    joo_add(info.requestData, "params", paramsObj);

    svc_ltmq_message_put(&info);

    jo_put(info.requestData);
    jo_put(info.replyData);

}
