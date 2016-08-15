/*
 *	hd_threadpool.c
 *  write by zouql 20120516
    
    zouqinglei@163.com 
    All right reserved.
 */

#include "lt_threadpool.h"

pthread_handler_decl(pool_sys_handle,arg)
{
    LT_ThreadInfo * pThread;
	LT_ThreadPool * pPool;

    pThread= (LT_ThreadInfo *)arg;
    pPool = (LT_ThreadPool *)pThread->parent;
    pThread->abort_loop = 0;

    printf("thread [%d] create.\n",pThread->thdID);

    pthread_mutex_lock(&pPool->mutex);
    pPool->activenum++;
    pthread_mutex_unlock(&pPool->mutex);

    
    while(!pThread->abort_loop)
    {
       pPool->userfunc(pThread);
    }
    
    pthread_mutex_lock(&pPool->mutex);
    pPool->activenum--;
    pThread->status = 0;
    pthread_mutex_unlock(&pPool->mutex);
    pthread_cond_signal(&pPool->cond);

    printf("thread [%d] end.\n",pThread->thdID);
    pthread_exit(0);
}

/************************************************************************/
/* thread pool func                                                     */
/************************************************************************/
LT_ThreadPool * LT_ThreadPool_new(int minsize, int maxsize, pthread_handler userfunc, void * userarg)
{
    int i;
    LT_ThreadPool * pPool;

    if(minsize > maxsize)
        return NULL;

    if((minsize < 0) || (minsize > LT_THREAD_POOL_MAX_SIZE))
        return NULL;

    if((maxsize < 0) || (maxsize > LT_THREAD_POOL_MAX_SIZE))
        return NULL;

    pPool = (LT_ThreadPool *)malloc(sizeof(LT_ThreadPool));
    if(!pPool)
        return NULL;

    BZERO(pPool,sizeof(LT_ThreadPool));

    pPool->threads = (LT_ThreadInfo *)malloc(sizeof(LT_ThreadInfo) * maxsize); 
    for(i = 0; i < maxsize; i++)
    {
        pPool->threads[i].status = 0;
        pPool->threads[i].parent = (void *)pPool;
        pPool->threads[i].thdID = 0;
        if(userarg != NULL)
            pPool->threads[i].userarg = userarg;
        else
            pPool->threads[i].userarg = (void *)i;

    }

    pthread_mutex_init(&pPool->mutex,NULL);
    pthread_cond_init(&pPool->cond,NULL);
    
    pPool->minsize = minsize;
    pPool->maxsize = maxsize;
    pPool->userfunc = userfunc;

    
    
    return pPool;
}

int LT_ThreadPool_start(LT_ThreadPool * pPool)
{
	int i;
	LT_ThreadInfo * pThread;

	pthread_mutex_lock(&pPool->mutex);
    for(i = 0; i < pPool->minsize; i++)
    {
        pThread = &pPool->threads[i];
        if(pthread_create(&pThread->thdID,NULL,pool_sys_handle,pThread)== 0)
        {
            pThread->status = 1;
        }
#ifdef WIN32
        Sleep(100);
#else
	    SLEEP(1);
#endif  
    }

    pthread_mutex_unlock(&pPool->mutex);

	return 0;
}


int LT_ThreadPool_stop(LT_ThreadPool * pPool)
{
	int i;
    struct timespec timeout;

    if(!pPool)
        return -1;

    for(i = 0; i < pPool->maxsize; i++)
    {
        if(pPool->threads[i].status == 1)
        {
            pPool->threads[i].abort_loop = 1;
        }
    }

    pthread_mutex_lock(&pPool->mutex);
    while(pPool->activenum > 0)
    {
        timeout.tv_sec = 1; 
        timeout.tv_nsec = 0;
        my_pthread_cond_timedwait(&pPool->cond,&pPool->mutex,&timeout);
    }
    pthread_mutex_unlock(&pPool->mutex);

	return 0;
}

int LT_ThreadPool_free(LT_ThreadPool * pPool)
{
    pthread_mutex_destroy(&pPool->mutex);

    pthread_cond_destroy(&pPool->cond);
    if(pPool->threads)
        free(pPool->threads);

    free(pPool);

    return 0;
}
/*
    ThreadPool_Inc()
    return threadID for success
    <= 0: failure
*/
int LT_ThreadPool_Inc(LT_ThreadPool * pPool,void * userarg)
{
    int i;
    LT_ThreadInfo * pThread;

    pthread_mutex_lock(&pPool->mutex);
    for(i = 0; i < pPool->maxsize; i++)
    {
        pThread = &pPool->threads[i];
        if(pThread->status == 0)
        {
            if(userarg != NULL)
                pThread->userarg = userarg;
            else
                pThread->userarg = (void *)i;
            if(pthread_create(&pThread->thdID,NULL,pool_sys_handle,pThread)== 0)
            {
                pThread->status = 1;
        
#ifdef WIN32
                Sleep(100);
#else
	            SLEEP(1);
#endif  
                break;
            }

        }
    }
    pthread_mutex_unlock(&pPool->mutex);
    if(i == LT_THREAD_POOL_MAX_SIZE)
        return -1;

    return (int)pThread->thdID;
}

int LT_ThreadPool_Dec(LT_ThreadPool * pPool, pthread_t thdID)
{
    int i;
    LT_ThreadInfo * pThread;
    pthread_mutex_lock(&pPool->mutex);
    for(i = 0; i < pPool->maxsize; i++)
    {
        pThread = &pPool->threads[i];
        if(pThread->status == 1)
        {
            if(thdID > 0)
            {
                if(thdID == pThread->thdID)
                {
                    pThread->abort_loop = 1;
                }
            }
            else if(thdID == 0)
            {
                pThread->abort_loop = 1;
                break;
            }
            else
            {
                pThread->abort_loop = 1;
            }
        }
    }
    pthread_mutex_unlock(&pPool->mutex);
   
    return 0;
}

int  LT_ThreadPool_Size(LT_ThreadPool * pPool)
{
    return pPool->activenum;
}

