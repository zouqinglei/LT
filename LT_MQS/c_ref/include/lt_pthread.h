/*
    pthread wrapped

    write by zouqinglei 20131113
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef LT_PTHREAD_H_
#define LT_PTHREAD_H_ 

#include "lt_commsocket.h"

#ifndef __WIN__
#define pthread_handler_decl(A,B) void * A(void *B)
typedef void * (*pthread_handler)(void *);
#endif

#ifdef __WIN__
#ifndef _WIN32_WCE
#define pthread_handler_decl(A,B) void  __cdecl  A(void *B)
typedef void    (__cdecl *pthread_handler)(void *);
#else
#define pthread_handler_decl(A,B) DWORD WINAPI A(LPVOID B)
typedef DWORD (*pthread_handler)(void *);
#endif /* _WIN32_WCE */

/*****************************************************************************
** The following is to emulate the posix thread interface
*****************************************************************************/

typedef HANDLE  pthread_t;
typedef struct thread_attr {
	DWORD	dwStackSize ;
	DWORD	dwCreatingFlag ;
	int	priority ;
} pthread_attr_t ;
typedef struct { int dummy; } pthread_condattr_t;
typedef struct {
        unsigned int waiting;
        HANDLE semaphore;
} pthread_cond_t;

#ifndef OS2

struct timespec {		/* For pthread_cond_timedwait() */
    time_t tv_sec;
    long tv_nsec;
};
#endif

typedef CRITICAL_SECTION pthread_mutex_t;

#define pthread_mutex_init(A,B)  InitializeCriticalSection(A)
#define pthread_mutex_lock(A)    (EnterCriticalSection(A),0)
#define pthread_mutex_unlock(A)  LeaveCriticalSection(A)
#define pthread_mutex_destroy(A) DeleteCriticalSection(A)

#define pthread_self() (HANDLE)GetCurrentThreadId()

/*
** We have tried to use '_beginthreadex' instead of '_beginthread' here
** but in this case the program leaks about 512 characters for each
** created thread !
*/

#ifdef  __cplusplus
extern "C" {
#endif 

int pthread_create(pthread_t *thread_id, pthread_attr_t *attr,
		   pthread_handler func, void *param);

int pthread_detach(pthread_t th);

int pthread_join(pthread_t threadId,void **status);

void pthread_exit(unsigned A);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

int pthread_cond_timedwait(pthread_cond_t *cond,pthread_mutex_t *mutex,
		struct timespec *abstime);

int my_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
							  struct timespec *abstime); /* abstime is the inter time for wait,not realy time */

int pthread_cond_signal(pthread_cond_t *cond);

int pthread_cond_destroy(pthread_cond_t *cond);

int pthread_attr_init(pthread_attr_t *connect_att);

int pthread_attr_setstacksize(pthread_attr_t *connect_att,DWORD stack);

int pthread_attr_setprio(pthread_attr_t *connect_att,int priority);

int pthread_attr_destroy(pthread_attr_t *connect_att);

#ifdef __cplusplus
}
#endif
#endif
#endif /* _my_ptread_h */
