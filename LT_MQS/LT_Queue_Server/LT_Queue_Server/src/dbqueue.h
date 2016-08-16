/*
 *	e_sqlitedb.h
    interface for sqlite3 db
	write by zouql 20080114
 */

#ifndef ECP_SQLITEDB_H_
#define ECP_SQLITEDB_H_

#include "sqlite3.h"
#include "e_pthread.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif
	
int DB_init();
int DB_destroy();

int DB_create_system_tables();

int DB_exist_queue(char *queue);
int DB_create_queue(HDQueueI *pQueueI);
int DB_delete_queue(char *queue);
int DB_update_queue(HDQueueI *pQueueI);

int DB_addMsg(char *queue,HDQMsg *pMsg);
int DB_getMsg(char *queue,HDQMsgGetOption *pGetOption,HDGetWait *pGetWait,HDQMsg **ppQMsg);

int DB_browsemsg(char *queue,int start,int count,HdmTable table);
int DB_queuesize(char *queue);
int DB_clear(char *queue);
int DB_delete_ttl(char *queue);

#ifdef __cplusplus
}
#endif

#endif /*ECP_SQLITEDB_H_*/

