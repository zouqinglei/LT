/*
 *	lt_mq_queue.h
    queue for sqlite database
	write by zouql 20140728
    zouqinglei@163.com
 */

#ifndef LTMQ_QUEUE_H_
#define LTMQ_QUEUE_H_

#include "lt_c_foundation.h"


#ifdef __cplusplus
extern "C" {
#endif 


/*
    queue type:
        'LOCAL'
        'TOPIC'
        'FORWARD'

    queueinfo json object is
    {
     'queue1':{'type':'LOCAL',
               'persistent':'N',
               'max_msgnum':99999,
               'max_msgsize':4194304,
               'create_time':'2014-07-28 13:39:23',
               'enable':'Y',
               'remark':'',
               
               --dynamic
               'current_msgnum":xxx,
               'wait_list':[],
              },
     'queue2':{
                ...
              }

    }

    request :
    headobj
    {
      "requestid":xxx,
      "type":'CREATE_QUEUE',
      "in_param":{}
      
     }

     reply:
     headobj
     {
        "requestid":xxx,
        "type":'CREATE_QUEUE',
        "out_param":{
            "errcod":"",
            "errstr":""
            } 
     }


*/


/*

    1. correlidwait:
        {
           "correlid1": [{"clientId":xxx, waitseconds},{"clientId":xxx, waitseconds}],
           "correlid2": {"clientId":xxx, waitseconds},{"clientId":xxx, waitseconds}],

        }
    2. tagwait
        {
          "tag1": [{"clientId":xxx, waitseconds},{"clientId":xxx, waitseconds}],
          "tag2": [{"clientId":xxx, waitseconds},{"clientId":xxx, waitseconds}],
                  
        }
    3. waitlist:
        [{"clientId":xxx, waitseconds},{"clientId":xxx, waitseconds}],
        
    get wait obj
    {
    "clientid":xxx,
    "waitseconds":'',
    }
*/


typedef struct stLTQueueInfo
{
	char * pszName;
	char * pszPersistent; /* 'Y' or 'N' */    
    int  max_msgnum;    /* max message number, default 9999 */
	int  max_msgsize;   /* max message size,   default 4M bytes */
    char * pszEnable;     /* 'Y' or 'N' */

    /* dynamic info */
    int current_msgnum;
    struct json_object * waitCorrelidObj; /* by correlid */
    struct json_object * waitTagObj; /* by tag */
    struct json_object * waitNoneListObj; /* wait list */

}LTQueueInfo;

/************************************************************************/
/* interface for queue                                                  */
/************************************************************************/ 
int ltmq_queue_init();
int ltmq_queue_destroy();

void inter_mq_put(const char * queueName, 
                         const char *correlid, 
                         const char * tag, 
                         int expiry, 
                         const char * title, 
                         const char * content);

#ifdef __cplusplus
}
#endif

#endif/*LTMQ_QUEUE_H_*/
