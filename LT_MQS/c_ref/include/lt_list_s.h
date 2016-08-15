/* ltlist.h
 * write by zouql 20131115
 * wrapped for mylinkedlist with mutex for safe,
 * just enable put and get method

    zouqinglei@163.com 
    All right reserved.
 */

#ifndef LT_LIST_SAFE_H
#define LT_LIST_SAFE_H

#include "lt_pthread.h"
#include "mylinkedlist_cache.h"

typedef struct stLT_LIST
{
    struct mylinkedlist * list;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int  max_size;
}LT_List;

#ifdef  __cplusplus
extern "C" {
#endif 

LT_List * LT_List_new(unsigned int max_size);

void  LT_List_free(LT_List * pList, void (*free_data_fn)(void *));
/*
 * LT_List_put 
 * put value data to the list pList
 * and signal the get thread
 * if the size > max_size return -1;
 * normally return 0
 */
int LT_List_put(LT_List * pList, void * data);

/*
 * LT_List_get
 * get a data value from the list pList,
 * if waitseconds is 0, then return immediately
 * if waitsecons > 0 and  the list is empty, then wait ,until 
 * timeout or has a data value 
 * if timeout, then return NULL
 */
void * LT_List_get(LT_List * pList, int waitseconds);

#ifdef __cplusplus
}
#endif

#endif /* LT_LIST_SAFE_H */
