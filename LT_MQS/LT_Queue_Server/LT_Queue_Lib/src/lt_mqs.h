/*
    lt_mqs.h
    write by zouql 20140811
    zouqinglei@163.com
    All right reversed.
*/

#ifndef LT_MQS_H_
#define LT_MQS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "json/json_short.h"

typedef void * LTMQHandle;

/*
    if flag is 1, then multiplex, if 0 ,then not multiplex, and no read thread
    in short link environment, this flag maybe set to 0
*/
LTMQHandle  ltmq_init(const char * hostIP, unsigned short port, int flag);

int ltmq_term(LTMQHandle hConn);

int ltmq_create(LTMQHandle hConn, 
                 const char * queue_name, 
                 int persistent,  /* 1 is db, 0 is memory */
                 int max_msgnum,
                 int max_msgsize,
                 const char * remark);

int ltmq_delete(LTMQHandle hConn, const char * queue_name);

int ltmq_clear(LTMQHandle hConn, const char * queue_name);

/*

if queue_name is null, then return all queue info.
recordset:
{
    "column":["a","b","c","d"],
    "data":[
         [  ],
         [  ]
        ]
}
*/


/*
message like 
{
    "correlid":"xxx",
    "tag":"xxx", 
    "expiry":10, 
    "title":"xxx", 
    "content":"xxx"
}
*/
json_object * ltmq_msg_new(const char * correlid, 
                 const char * tag, 
                 int expiry, 
                 const char * title, 
                 const char * content);


int ltmq_put(LTMQHandle hConn, 
             const char * queue_name, 
             json_object * msgObj);
       
int ltmq_get(LTMQHandle hConn,
             const char * queue_name,
             const char * correlid,
	         const char * tag,
             json_object * msgObj,
             int waitseconds);



/*
    recordset:
    {"column":["a","b","c","d"],
     "data":[
             [  ],
             [  ]
            ]
    }

    use LTRecord function process the result.
*/

int ltmq_browse(LTMQHandle hConn,
                const char * queue_name,
                int start,
                int limit,
                json_object * recordObj);

int ltmq_browse_msg(LTMQHandle hConn,
                const char * queue_name,
                const char * msgid,
                json_object * msgObj);


int ltmq_info(LTMQHandle hConn,
              const char * type,
              const char * name,
              json_object * recordObj);



/* server def */

#define  LT_CONST_SHORT  40

typedef struct stltsvcinfo
{
    const char * name;	/* Service name  */
    json_object * paramObj;			
	json_object * resultObj;     
    int  flag;			/* Service attributes */
    int reply;   /* 1: need reply, 0: don't reply */
}LtSvcInfo;

typedef void (*LtSvcFunc)(LtSvcInfo * pInfo);

void ltns_register(const char * service, LtSvcFunc fp);

int ltns_serverloop(const char * module, int argc,char *argv[]);

/*
    paramObj like 
    {
      "param1":"xxx",
      "param2":"xxx",
    }

    resultObj like:
    {
      "errcod": "SUCCESS",
      "errstr": "",
      "data": {}
    }
*/
int ltns_call(LTMQHandle hMQ,
              const char * module,
              const char * service, 
              json_object * paramObj,
              json_object * resultObj,
              int waitseconds);


#ifdef __cplusplus
}
#endif

#endif /* LT_MQS_H_ */
