/*
 *	lt_threadpool.h
 *  write by zouql 20120516
 *  wrap a thread struct ,for use ealisy
 *  
 *  20080714  add threadpool
 *  
 *  20080825 splite from e_thread module,add task queue process
 *  20131115 modify by zouql for lt queue

    zouqinglei@163.com 
    All right reserved.
 */

#ifndef LT_THREADPOOL_H_
#define LT_THREADPOOL_H_

#include "lt_pthread.h"

#define LT_THREAD_POOL_MAX_SIZE 1024

typedef struct stLT_ThreadInfo
{
    int status;   /* 0:free 1:run*/
    int  abort_loop;
    pthread_t thdID;
        
    pthread_handler handler;

    void * parent;

    void * userarg;

}LT_ThreadInfo;

typedef struct stLT_ThreadPool
{
    LT_ThreadInfo * threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int minsize; 
    int maxsize;

    pthread_handler userfunc;

    int activenum;

}LT_ThreadPool;

#ifdef  __cplusplus
extern "C" {
#endif 
/* interface for client */
LT_ThreadPool * LT_ThreadPool_new(int minsize, int maxsize, pthread_handler userfunc, void * userarg);
int LT_ThreadPool_start(LT_ThreadPool * pPool);
int LT_ThreadPool_stop(LT_ThreadPool * pPool);
int LT_ThreadPool_free(LT_ThreadPool * pPool);

/*
    ThreadPool_Inc()
    return threadID for success
    <= 0: failure
*/
int LT_ThreadPool_Inc(LT_ThreadPool * pPool,void * userarg);
/*
    ThreadPool_Dec()
    pthread_t thdID, -1 for every one
    > 0, for special thdID thread stopped
*/
int LT_ThreadPool_Dec(LT_ThreadPool * pPool, pthread_t thdID);
int  LT_ThreadPool_Size(LT_ThreadPool * pPool);


#ifdef __cplusplus
}
#endif

#endif /* HD_THREADPOOL_H_ */

