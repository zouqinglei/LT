/*
 *	lt_singlethread.h
 *  write by zouql 20131113
 *  wrap a thread struct ,for use ealisy
 *  zouqinglei@163.com 
    All right reserved. 
 */

#ifndef LT_SINGLE_THREAD_H_
#define LT_SINGLE_THREAD_H_

#include "lt_pthread.h"


typedef struct stSingleThread
{
    int  abort_loop;
    pthread_t thdID;
        
    pthread_handler handler;
    void * userarg;
    
    int exit_flag;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

}LT_Thread;


#ifdef  __cplusplus
extern "C" {
#endif 
/* interface for client */
LT_Thread * LT_Thread_new(pthread_handler handler, void * userarg);
int LT_Thread_free(LT_Thread * pThread);

int LT_Thread_start(LT_Thread * pThread);
int LT_Thread_stop(LT_Thread * pThread);
int LT_Thread_exit(LT_Thread * pThread);

#ifdef __cplusplus
}
#endif

#endif /* LT_SINGLE_THREAD_H_ */

