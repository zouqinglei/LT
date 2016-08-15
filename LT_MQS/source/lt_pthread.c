/*
    zouqinglei@163.com 
    All right reserved.
*/

/*****************************************************************************
** The following is to emulate the posix thread interface
*****************************************************************************/
#include "lt_pthread.h"

/*
** We have tried to use '_beginthreadex' instead of '_beginthread' here
** but in this case the program leaks about 512 characters for each
** created thread !
*/
#ifdef __WIN__
#ifndef _WIN32_WCE
int pthread_create(pthread_t *thread_id, pthread_attr_t *attr,
		   pthread_handler func, void *param)
{
  HANDLE hThread;
  if(attr)
     hThread=(HANDLE)_beginthread(func,
			       attr->dwStackSize ? attr->dwStackSize :
			       65535,param);
  else
	  hThread = (HANDLE)_beginthread(func,65535,param);
  if (((long) hThread) == -1L)
  {
      printf("........create thread errno:%d\n",errno);
    return(errno ? errno : -1);
  }
  *thread_id=hThread;
  return(0);
}
#else /* for WIN32_WCE */
int pthread_create(pthread_t *thread_id, pthread_attr_t *attr,
		   pthread_handler func, void *param)
{
  HANDLE hThread;
  
  hThread = CreateThread(NULL,0,func,param,0,(unsigned long *)thread_id);
  if ((long) hThread == -1L)
  {
    return -1;
  }
  
  return(0);
}
#endif


int pthread_detach(pthread_t th)
{
    return 0;
}

int pthread_join(pthread_t threadId,void **status)
{
	int  ret;
	
	ret= (int) WaitForSingleObject(threadId,INFINITE);

	return ret;

	return 0;
}

void pthread_exit(unsigned A)
{
#ifndef _WIN32_WCE
  _endthread();
#else
  ExitThread(0);
#endif
}

/*
** The following simple implementation of conds works as long as
** only one thread uses pthread_cond_wait at a time.
** This is coded very carefully to work with thr_lock.
*/
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    cond->waiting = 0;
	cond->semaphore = CreateSemaphore(NULL,0,INT_MAX,NULL);
	if(!cond->semaphore)
		return -1;
  return 0;
}


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    InterlockedIncrement(&cond->waiting);
	LeaveCriticalSection(mutex);
    WaitForSingleObject(cond->semaphore,INFINITE);
	InterlockedDecrement(&cond->waiting);
	EnterCriticalSection(mutex);
	return 0 ;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           struct timespec *abstime)
{

  int result;
  long timeout;

  timeout = ((long) (abstime->tv_sec)*1000L +
		    (long)((abstime->tv_nsec/1000))/1000L);
#ifndef _WIN32_WCE
  timeout = timeout - ( time(NULL) * 1000 );
#endif
  if (timeout < 0)				/* Some safety */
    timeout = 0L;
  InterlockedIncrement(&cond->waiting);
  LeaveCriticalSection(mutex);
  result=WaitForSingleObject(cond->semaphore,timeout);
  InterlockedDecrement(&cond->waiting);
  EnterCriticalSection(mutex);

  return result == WAIT_TIMEOUT ? ETIMEDOUT : 0;
}


int pthread_cond_signal(pthread_cond_t *cond)
{
   long prev_count;

   if(cond->waiting)
	   ReleaseSemaphore(cond->semaphore,1,&prev_count);
   
  return 0 ;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	return CloseHandle(cond->semaphore)?0:-1;
}

int pthread_attr_init(pthread_attr_t *connect_att)
{
  connect_att->dwStackSize	= 0;
  connect_att->dwCreatingFlag	= 0;
  connect_att->priority		= 0;
  return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *connect_att,DWORD stack)
{
  connect_att->dwStackSize=stack;
  return 0;
}

int pthread_attr_setprio(pthread_attr_t *connect_att,int priority)
{
  connect_att->priority=priority;
  return 0;
}

int pthread_attr_destroy(pthread_attr_t *connect_att)
{
  return 0;
}

#endif


int my_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           struct timespec *abstime)
{
#ifdef __WIN__	
	int result;
	long timeout;
	
	timeout = ((long) (abstime->tv_sec)*1000L +
		(long)((abstime->tv_nsec/1000))/1000L);
	
	if (timeout < 0)				/* Some safety */
		timeout = 0;
	InterlockedIncrement(&cond->waiting);
	LeaveCriticalSection(mutex);
	result=WaitForSingleObject(cond->semaphore,timeout);
	InterlockedDecrement(&cond->waiting);
	EnterCriticalSection(mutex);
	
	return result == WAIT_TIMEOUT ? ETIMEDOUT : 0;

#else
	struct timespec ts;
	struct timeval now;
	long sec;

	n = 0;
	gettimeofday(&now,NULL);
	ts.tv_sec = now.tv_sec + abstime->tv_sec;
	n = (long)((now.tv_usec * 1000 + abstime->tv_nsec)/(1000 * 1000 * 1000));
	if(n > 0)
		ts.tv_sec += 1;
	ts.tv_nsec = now.tv_usec * 1000 + abstime->tv_nsec;;
	return pthread_cond_timedwait(cond,mutex,&ts);
#endif
};
