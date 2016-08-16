/*
    forward.h
    forward base on queue
    write by zouql 20081107

    1.first version,dispatch thread one forward
*/

#ifndef E_FORWARD_17_H_
#define E_FORWARD_17_H_


#include "e_global.h"
#include "e_pthread.h"
#include "mba/linkedlist.h"
#include "e_util.h"
#include "e_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    char host[HDM_CONST_SHORT];
    char queue[HDM_CONST_SHORT];
    int  weight;            /* weight for this queue, 0,none,-1:must be forward no conditions*/
    int  num;               /* where have forward message num */
}HDForwardDst;

typedef struct stForward
{
    void *pQueueI;   

    struct linkedlist *pDstList; /* HDForwardDst */
    
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    EThread *pDispatchThread;
}HDForward;

HDForwardDst *HDForwardDst_new(char *host,char *queue,int weight);
int HDForwardDst_free(HDForwardDst *pDst);

HDForward *HDForward_new(void *pQueueI);
int HDForward_free(HDForward *pForward);
int HDForward_buildFromDB(HDForward *pForward);
int HDForward_signal(HDForward *pForward);
pthread_handler_decl(handle_forward_dispatch,arg);

int DB_add_forwardDst(char *forwardQueue,HDForwardDst *pDst);
int DB_del_forwardDst(char *forwardQueue,HDForwardDst *pDst);
int DB_upd_forwardDst_weight(char *forwardQueue,HDForwardDst *pDst);

HDForwardDst *HDForward_find(HDForward *pForward,char *host,char *queue);
/* interface */
int HDForward_AddDst(HDForward *pForward,char *host,char *queue,int weight);
int HDForward_DelDst(HDForward *pForward,char *host,char *queue);
int HDForward_UpdDstWeight(HDForward *pForward,char *host,char *queue,int weight);
int HDForward_browse(HDForward *pForward,HdmTable table);
#ifdef __cplusplus
}
#endif

#endif /*E_TOPIC_17_H_*/

