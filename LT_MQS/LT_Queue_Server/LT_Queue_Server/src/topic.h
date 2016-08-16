/*
    topic.h
    topic base on queue
    write by zouql 20081107

    1.first version,dispatch thread one topic, topic client set manage
*/

#ifndef E_TOPIC_17_H_
#define E_TOPIC_17_H_


#include "e_global.h"
#include "e_pthread.h"
#include "mba/linkedlist.h"
#include "e_util.h"
#include "e_thread.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct stTopic
{
    void *pQueueI;
    EcpIDArray *pClientIDArray; /* clientID array */
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    EThread *pDispatchThread;
}HDTopic;

HDTopic *HDTopic_new(void *pQueueI);
int HDTopic_free(HDTopic *pTopic);
int HDTopic_addclient(HDTopic *pTopic,int clientID);
int HDTopic_delclient(HDTopic *pTopic,int clientID);
int HDTopic_signal(HDTopic *pTopic);
pthread_handler_decl(handle_topic_dispatch,arg);

#ifdef __cplusplus
}
#endif



#endif /*E_TOPIC_17_H_*/

