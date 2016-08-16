/*
 *	e_sqlitedb.c
    interface for sqlite3 db
	write by zouql 20080114
 */

#include "e_global.h"
#include "e_util.h"

#include "dbqueue.h"

sqlite3 *db;   /* sqltie3 db interface */


int DB_init()
{
	int result;

    char pathname[MAX_PATH];
    char queue_dbfile[MAX_PATH];

#ifdef WIN32
	GetModuleFileName(NULL,pathname,MAX_PATH);
    sprintf(queue_dbfile,"%s\\%s",e_dirname(pathname),"hdmq.db");
#else
    sprintf(pathname,"%s", getenv("HDM_PATH"));
    sprintf(queue_dbfile,"%s/%s",pathname,"/hdmq.db");
#endif

	db = NULL;
	result = sqlite3_open(queue_dbfile,&db);

	if(result != SQLITE_OK)
		return EEFAIL;
	DB_create_system_tables();
	/* create system queue */
	return 0;
}

int DB_destroy()
{
    if(db != NULL)
		sqlite3_close(db);

	return 0;
}

/* a table for all queue's information */
int DB_create_system_tables()
{
	int result;
	char sql[1024];

    sprintf(sql,"CREATE \
					TABLE SYS_QUEUE_INFO(NAME VARCHAR(40) PRIMARY KEY NOT NULL, \
					TYPE INT NOT NULL DEFAULT 0, \
                    PERSISTENT INT NOT NULL DEFAULT 0, \
					MAX_MSGNUM INT NOT NULL DEFAULT 99999, \
					MAX_MSGSIZE INT NOT NULL DEFAULT 4194304, \
					CREATE_TIME INT NOT NULL, \
                    ENABLE INT NOT NULL DEFAULT 1, \
                    REMARK VARCHAR(255))");
	
	result = sqlite3_exec(db,sql,NULL,NULL,NULL);
	if(result != SQLITE_OK)
		printf("create table sys_queue_info error:(%d)\n",result);
	
    /* create some default queue ,such as broadcast,NamSvcRequest,NamSvcReply */
    sprintf(sql,"INSERT INTO SYS_QUEUE_INFO VALUES('broadcast',3,0,99999,4194304,%d,1,'broadcast topic queue')",
        (long)time(NULL));
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);

    sprintf(sql,"INSERT INTO SYS_QUEUE_INFO VALUES('NamSvcRequest',0,0,99999,4194304,%d,1,'name service request queue')",
        (long)time(NULL));
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);

    sprintf(sql,"INSERT INTO SYS_QUEUE_INFO VALUES('NamSvcReply',0,0,99999,4194304,%d,1,'name service reply queue')",
        (long)time(NULL));
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);

    /* create channel info table */
    sprintf(sql,"CREATE \
TABLE SYS_CHANNEL_INFO(DSTHOST VARCHAR(40) NOT NULL DEFAULT '127.0.0.1', \
DSTPORT INT NOT NULL DEFAULT 0, \
ENABLE INT NOT NULL DEFAULT 1, \
REMARK VARCHAR(255))");

     result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result != SQLITE_OK)
        printf("create table SYS_CHANNEL_INFO error:(%d)\n",result);

    /* create forward info table */    
    sprintf(sql,"CREATE \
TABLE SYS_FORWARD_INFO(FORWARD_QUEUE VARCHAR(40) NOT NULL, \
DSTHOST VARCHAR(40) NOT NULL DEFAULT '127.0.0.1', \
DSTQUEUE VARCHRA(40) NOT NULL, \
WEIGHT INT DEFAULT 1)");
        
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);
    if(result != SQLITE_OK)
        printf("create table SYS_FORWARD_INFO error:(%d)\n",result);

	return 0;
}

int DB_exist_queue(char *queue)
{
	int result;
	sqlite3_stmt *stmt;
	char sql[1024];
    int count;
	int ret;

	if(!queue)
		return EEFAIL;
	
	/**/
	count = -1;
	ret = EEFAIL;
    sprintf(sql,"SELECT COUNT(*) FROM SYS_QUEUE_INFO WHERE NAME = '%s'",queue);
    result = sqlite3_prepare(db,sql,-1,&stmt,0);
	if(result == SQLITE_OK)
	{
		result = sqlite3_step(stmt);
		if(result == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt,0);
		}
	}
    sqlite3_finalize(stmt);
	
    if(count == -1)
		ret = EEFAIL;
	else
		ret = count;
    
	return ret;
}

/*
 *	create a queue in sqlite3db,first insert into SYS_QUEUE_INFO a item,
 *  if exist,return EEEXIST
 *  if queue's persistent is 'F',then create a table as the same name of queue for store persistend msg
 *  return 0 is success.
 */
int DB_create_queue(HDQueueI *pQueueI)
{
	int result;
    char *errmsg;
	
	char sql[1024];
    int count;
    int ret;
	
	errmsg = NULL;
    ret = EEFAIL;
	
	count = DB_exist_queue(pQueueI->Queue.name);
	if(count == 0)
    {
        result = sqlite3_exec(db,"begin transaction",NULL,NULL,NULL);
        
		sprintf(sql,"INSERT INTO SYS_QUEUE_INFO VALUES('%s',%d,%d,%d,%d,%d,%d,'%s')",
			pQueueI->Queue.name,
            pQueueI->Queue.type,
            pQueueI->Queue.persistent,
			pQueueI->Queue.max_msgnum,
			pQueueI->Queue.max_msgsize,
            (long)time(NULL),
            pQueueI->enable,
            pQueueI->Queue.remark);
		result = sqlite3_exec(db,sql,NULL,NULL,NULL);
		if(result == SQLITE_OK)
		{
            if(pQueueI->Queue.persistent == HDMQ_PERSISTENT_FILE)
            {
                sprintf(sql,"CREATE TABLE %s ( \
                MSGID VARCHAR(40), \
				CORRELID VARCHAR(40), \
				TYPE INT NOT NULL DEFAULT 0, \
                FORMAT INT NOT NULL DEFAULT 0, \
				EXPIRY INT NOT NULL DEFAULT 0, \
				PRIORITY NOT NULL DEFAULT 0, \
                REPLYQ VARCHAR(40), \
				REPLYQM VARCHAR(40), \
                APPLICATION VARCHAR(254), \
				CREATE_TIME INT NULL, \
                USERTAG INT NOT NULL DEFAULT 0, \
                USERREMARK VARCHAR(254), \
				DATASIZE  INT NOT NULL DEFAULT 0, \
				DATA BLOB)",pQueueI->Queue.name);
            
                result = sqlite3_exec(db,sql,NULL,NULL,NULL);
                if(result == SQLITE_OK)
                {
                    ret = 0;
                }
            }
            else
            {
                ret = 0;
            }
		
		}
		
		if(ret == 0)
			result = sqlite3_exec(db,"commit transaction",NULL,NULL,NULL);
		else
			result = sqlite3_exec(db,"rollback transaction",NULL,NULL,NULL);
    }
    else
    {
        ret = EEEXIST;
    }
	

	return ret;
}

int DB_delete_queue(char *queue)
{
	int result;
    char *errmsg;
	char sql[1024];
    int count;
	int ret;
	
	if(!queue)
		return EEFAIL;
	
    ret = EEFAIL;
	count = DB_exist_queue(queue);
    errmsg = NULL;
	if(count == 1)
	{
		result = sqlite3_exec(db,"begin transaction",NULL,NULL,NULL);
		
		sprintf(sql,"DELETE FROM SYS_QUEUE_INFO WHERE NAME = '%s'",queue);
		result = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(result == SQLITE_OK)
		{
			sprintf(sql,"DROP TABLE %s ",queue);
			
			result = sqlite3_exec(db,sql,NULL,NULL,NULL);
			ret = 0;
		}	

        sprintf(sql,"DELETE FROM SYS_FORWARD_INFO WHERE FORWARD_QUEUE = '%s'",queue);

        result = sqlite3_exec(db,sql,NULL,NULL,NULL);

		if(ret == 0)
			result = sqlite3_exec(db,"commit transaction",NULL,NULL,NULL);
		else
			result = sqlite3_exec(db,"rollback transaction",NULL,NULL,NULL);
	}
    else
    {
        ret = 0;
    }
	
	
	return ret;
}

int DB_update_queue(HDQueueI *pQueueI)
{
	int result;
    char *errmsg;
	char sql[1024];
    int count;
	int ret;
	
    ret = EEFAIL;
	count = DB_exist_queue(pQueueI->Queue.name);
    errmsg = NULL;
	if(count == 1)
	{
		sprintf(sql,"UPDATE SYS_QUEUE_INFO SET MAX_MSGNUM = %d, \
 MAX_MSGSIZE = %d , REMARK = '%s'  WHERE NAME = '%s'",
        pQueueI->Queue.max_msgnum,
        pQueueI->Queue.max_msgsize,
        pQueueI->Queue.remark,
        pQueueI->Queue.name);
		result = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
		if(result != SQLITE_OK)
		{
			printf("udpate table sys_queue_info error:(%d)\n",result);
		}
        else
        {
            ret = 0;
        }

    }
	
	return ret;
}

int DB_addMsg(char *queue,HDQMsg *pMsg)
{
	int result;
	sqlite3_stmt *stmt;
	char sql[1024];
    int count;
	int ret;

	
	ret = EEFAIL;

    count = 1;
    if(count == 1)
	{
		sprintf(sql,"INSERT \
			INTO %s VALUES('%s','%s',%d,%d,%d,%d,'%s','%s','%s', \
			%d, \
			%d,'%s',%d,?)",queue,
			pMsg->hdr.msgId,
            pMsg->hdr.correlId,
            pMsg->hdr.type,
            pMsg->hdr.format,
            pMsg->hdr.expiry,
            pMsg->hdr.priority,
            pMsg->hdr.replyToQ,
            pMsg->hdr.replyToQMgr,
            pMsg->hdr.application,
            pMsg->hdr.createtime,
            pMsg->hdr.userTag,
            pMsg->hdr.userRemark,
            pMsg->size);
		result = sqlite3_prepare(db,sql,-1,&stmt,NULL);
		if(result == SQLITE_OK)
		{
			result = sqlite3_bind_blob(stmt,1,pMsg->blob,pMsg->size,NULL);
			result = sqlite3_step(stmt);
			if(result == SQLITE_DONE)
				ret = 0;
		}
		sqlite3_finalize(stmt);
	}
	
	return ret;
}

/* 
   DB_getMsg
   get only one message 
   retrieve by usertag,userremark,priority or default first one
*/
int DB_getMsg(char *queue,HDQMsgGetOption *pGetOption,HDGetWait *pGetWait,HDQMsg **ppQMsg)

{
    int ret;
	int result;
	char *errmsg;
	sqlite3_stmt *stmt;
	char sql[1024];
    char where[1024];
    int col;
    int rowid;
    HDQMsg *pMsg;
	
	errmsg = NULL;
    ret = EENODATA;
    pMsg = NULL;

    if(pGetWait->MatchOptions == HDMQ_GETOPT_MATCH_NONE)
        sprintf(where,"");
    else
    {
        if(pGetWait->MatchOptions & HDMQ_GETOPT_MATCH_MSG_ID)
            sprintf(where, " WHERE MSGID = '%s' ",pGetWait->msgId);
        else if(pGetWait->MatchOptions & HDMQ_GETOPT_MATCH_CORREL_ID)
            sprintf(where, " WHERE CORRELID = '%s' ",pGetWait->msgId);
        else if(pGetWait->MatchOptions & HDMQ_GETOPT_MATCH_USER_TAG)
            sprintf(where," WHERE USERTAG = %d ",pGetWait->userTag);
    }

    sprintf(sql,"SELECT ROWID,MSGID,CORRELID,TYPE,FORMAT,EXPIRY,PRIORITY,REPLYQ,REPLYQM,APPLICATION, \
        CREATE_TIME,USERTAG,USERREMARK,DATASIZE,DATA FROM %s %s LIMIT 1",queue,where);


    result = sqlite3_prepare(db,sql,-1,&stmt,NULL);
	if(result == SQLITE_OK)
	{
		result = sqlite3_step(stmt);
		if(result == SQLITE_ROW)
		{
            ret = 0;
            pMsg = HDQMsg_new();
            col = 0;
            rowid = sqlite3_column_int(stmt,col++);
            strcpy(pMsg->hdr.msgId,(char *)sqlite3_column_text(stmt,col++));
            strcpy(pMsg->hdr.correlId,(char *)sqlite3_column_text(stmt,col++));
            pMsg->hdr.type   = sqlite3_column_int(stmt,col++);
            pMsg->hdr.format = sqlite3_column_int(stmt,col++);
            pMsg->hdr.expiry = sqlite3_column_int(stmt,col++);
            pMsg->hdr.priority = sqlite3_column_int(stmt,col++);
            strcpy(pMsg->hdr.replyToQ,(char *)sqlite3_column_text(stmt,col++));
            strcpy(pMsg->hdr.replyToQMgr,(char *)sqlite3_column_text(stmt,col++));
            strcpy(pMsg->hdr.application,(char *)sqlite3_column_text(stmt,col++));
            pMsg->hdr.createtime = sqlite3_column_int(stmt,col++);
            pMsg->hdr.userTag = sqlite3_column_int(stmt,col++);
            strcpy(pMsg->hdr.userRemark,(char *)sqlite3_column_text(stmt,col++));
            pMsg->size = sqlite3_column_int(stmt,col++);
            pMsg->blob = NEW(pMsg->size);
            BCOPY(sqlite3_column_blob(stmt,col++),pMsg->blob,pMsg->size);
		}
	}

	sqlite3_finalize(stmt);

    if(pMsg)
    {
        if(pMsg->size > pGetWait->clientbuflen)
        {
            if(!(pGetOption->Options & HDMQ_GETOPT_ACCEPT_TRUNCATED_MSG))
            {
                HDQMsg_free(pMsg);
                pMsg = NULL;
                ret = EELARGEDATA;
            }
        }
    }

    if(ret == 0)
    {
        if(!(pGetOption->Options & HDMQ_GETOPT_BROWSE))
        {
            sprintf(sql,"DELETE FROM %s WHERE ROWID = %d",queue,rowid);
            result = sqlite3_exec(db,sql,NULL,NULL,NULL);
        }
        *ppQMsg = pMsg;
    }

	return ret;
}

int DB_browsemsg(char *queue,int start,int count,HdmTable table)
{
    int ret;
	int result;
	char *errmsg;
	sqlite3_stmt *stmt;
	char sql[1024];
    int i;
    int row;
    int col;
    char tmpbuf[HDM_CONST_LONG];
    int time;

    ret = 0;
	errmsg = NULL;

    if(start < 0)
        start = 0;
    if(count < 0)
        count = 99999;
	sprintf(sql,"SELECT MSGID,CORRELID,TYPE,FORMAT,EXPIRY,PRIORITY, \
        REPLYQ,REPLYQM,APPLICATION, \
        CREATE_TIME,USERTAG,USERREMARK,DATASIZE \
        FROM %s LIMIT %d,%d",queue,start,count);

    result = sqlite3_prepare(db,sql,-1,&stmt,NULL);
	if(result == SQLITE_OK)
    {
        while((result = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            i = 0;
            col = 0;
            
            row = hdt_insertrow(table,-1);
            
            hdt_setitemstring(table,row,col++,(char *)sqlite3_column_text(stmt,i++));
            hdt_setitemstring(table,row,col++,(char *)sqlite3_column_text(stmt,i++));
            
            sprintf(tmpbuf,"%d",sqlite3_column_int(stmt,i++));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
            sprintf(tmpbuf,"%d",sqlite3_column_int(stmt,i++));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
            sprintf(tmpbuf,"%d",sqlite3_column_int(stmt,i++));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
            sprintf(tmpbuf,"%d",sqlite3_column_int(stmt,i++));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
            hdt_setitemstring(table,row,col++,(char *)sqlite3_column_text(stmt,i++));
            hdt_setitemstring(table,row,col++,(char *)sqlite3_column_text(stmt,i++));
            hdt_setitemstring(table,row,col++,(char *)sqlite3_column_text(stmt,i++));
            
            time = (time_t)sqlite3_column_int(stmt,i++);
            sprintf(tmpbuf,"%s",e_strftime((time_t *)&time));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
            sprintf(tmpbuf,"%d",sqlite3_column_int(stmt,i++));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
            hdt_setitemstring(table,row,col++,(char *)sqlite3_column_text(stmt,i++));
            
            sprintf(tmpbuf,"%d",sqlite3_column_int(stmt,i++));
            hdt_setitemstring(table,row,col++,tmpbuf);
            
        }
	}

	sqlite3_finalize(stmt);
     
    return ret;
}

int DB_queuesize(char *qname)
{
    int ret;
    int result;
    sqlite3_stmt *stmt;
    char *errmsg;
    char sql[1024];
    int count;
    
    if(qname == NULL )
        return 0;
    
    ret = 0;
    errmsg = NULL;
    count = 0;

    sprintf(sql,"SELECT COUNT(*) FROM %s",qname);
    result = sqlite3_prepare(db,sql,-1,&stmt,0);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        if(result == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt,0);
        }
    }
    sqlite3_finalize(stmt);
    
    ret = count;
    return ret;
}

int DB_clear(char *queue)
{
    int result;
	char sql[1024];

    sprintf(sql,"DELETE FROM %s",queue);
	
	result = sqlite3_exec(db,sql,NULL,NULL,NULL);
	if(result != SQLITE_OK)
		printf("delete table %s error:(%d)\n",queue,result);

    return 0;
}

int DB_delete_ttl(char *queue)
{
    int ret;
    int result;
    char *errmsg;
    char sql[1024];
    time_t now;
    
    if(queue == NULL )
        return 0;
    ret = 0;
    errmsg = NULL;
    
    now = time(NULL);
    sprintf(sql,"DELETE FROM %s WHERE EXPIRY > 0 AND %d - CREATE_TIME >= EXPIRY",queue,now);
    result = sqlite3_exec(db,sql,NULL,NULL,NULL);

    if(result == SQLITE_OK)
    {
        ret = sqlite3_changes(db);
    }
   
    return ret;
}

