/*
    zouqinglei@163.com 
    All right reserved.
*/

#include "lt_list_s.h"

LT_List * LT_List_new(unsigned int max_size)
{
    LT_List * pList = (LT_List *)malloc(sizeof(LT_List));
    pList->list = mylinkedlist_new(max_size);
    pList->max_size = max_size;
    pthread_mutex_init(&pList->mutex,NULL);
    pthread_cond_init(&pList->cond,NULL);

    return pList;
}


void  LT_List_free(LT_List * pList, void (*free_data_fn)(void *))
{
    if(pList == NULL)
        return;
    mylinkedlist_del(pList->list, free_data_fn);
    pthread_mutex_destroy(&pList->mutex);
    pthread_cond_destroy(&pList->cond);


}

/*
 * LT_List_put 
 * put value data to the list pList
 * and signal the get thread
 * if the size > max_size return -1;
 * normally return 0
 */
int LT_List_put(LT_List * pList, void * data)
{
    int ret;

    ret = 0;

    pthread_mutex_lock(&pList->mutex);
    ret = mylinkedlist_add(pList->list, data);
    pthread_mutex_unlock(&pList->mutex);
    pthread_cond_signal(&pList->cond);

    return ret;
}

/*
 * LT_List_get
 * get a data value from the list pList,
 * if waitseconds is 0, then return immediately
 * if waitsecons > 0 and  the list is empty, then wait ,until 
 * timeout or has a data value 
 * if waitsecons is -1, then wait infinate
 * if timeout, then return NULL
 */
void * LT_List_get(LT_List * pList, int waitseconds)
{
    void * data;
    int retval;
    struct timespec ts;

    data = NULL;
    if(pList == NULL)
        return NULL;

    pthread_mutex_lock(&pList->mutex);
    if(mylinkedlist_size(pList->list) > 0)
    {
        data = mylinkedlist_remove(pList->list,0);
    }
    else
    {
        if(waitseconds == 0)
        {
        
        }
        else if(waitseconds < 0)
        {
            retval = pthread_cond_wait(&pList->cond,&pList->mutex);
            if(mylinkedlist_size(pList->list) > 0)
            {
                data = mylinkedlist_remove(pList->list,0);
            }
        }
        else if(waitseconds > 0)
        {
            ts.tv_sec = waitseconds;
            ts.tv_nsec = 0;
            retval = my_pthread_cond_timedwait(&pList->cond,&pList->mutex,&ts);
            if(retval == 0 && mylinkedlist_size(pList->list) > 0)
            {
                data = mylinkedlist_remove(pList->list,0);
            }
        }
    }

    pthread_mutex_unlock(&pList->mutex);
    
    return data;

}
