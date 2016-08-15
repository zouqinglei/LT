/*
 *	lt_threadpool.c
 *  write by zouql 20120516
     
    zouqinglei@163.com 
    All right reserved.
 */

#include "lt_singlethread.h"


/************************************************************************/
/* thread pool func                                                     */
/************************************************************************/
LT_Thread * LT_Thread_new(pthread_handler handler, void * userarg)
{
    LT_Thread * pThread;

    if(!handler)
        return NULL;
    pThread = (LT_Thread *)malloc(sizeof(LT_Thread));
    pThread->abort_loop = 0;
    pThread->handler = handler;
    pThread->userarg = userarg;
    pThread->exit_flag = 0;
    pthread_mutex_init(&pThread->mutex,NULL);
    pthread_cond_init(&pThread->cond,NULL);
    
    return pThread;
}

int LT_Thread_free(LT_Thread * pThread)
{
    if(!pThread)
        return -1;
    pthread_mutex_destroy(&pThread->mutex);
    pthread_cond_destroy(&pThread->cond);
    
    free(pThread);

    return 0;
}

int LT_Thread_start(LT_Thread * pThread)
{
    if(!pThread)
        return -1;
    if(pthread_create(&pThread->thdID,NULL,pThread->handler,pThread->userarg) == 1)
        return 0;

    return -1;
}

int LT_Thread_stop(LT_Thread * pThread)
{
    pThread->abort_loop = 1;
    pthread_mutex_lock(&pThread->mutex);
    while(!pThread->exit_flag)
    {
        pthread_cond_wait(&pThread->cond,&pThread->mutex);
    }
    pthread_mutex_unlock(&pThread->mutex);

    return 0;
}

int LT_Thread_exit(LT_Thread * pThread)
{
    pthread_mutex_lock(&pThread->mutex);
    pThread->exit_flag = 1;   
    pthread_cond_signal(&pThread->cond);  
    pthread_mutex_unlock(&pThread->mutex);

    pthread_exit(0);

    return 0;
}
