/*

*/

#include "lt_methods.h"


/************************************************************
 *    global data declare
 ************************************************************/
static struct json_object * g_pMethodsObj;
static pthread_mutex_t mutex;

/************************************************************
 * end global data declare
 ************************************************************/
int lt_methods_init()
{
    g_pMethodsObj = jo_new_object();
    pthread_mutex_init(&mutex, NULL);
    return 0;
}

int lt_methods_destroy()
{
    jo_put(g_pMethodsObj);
    pthread_mutex_destroy(&mutex);
    return 0;
}

int lt_methods_register(const char * name, serverProcessFunc func)
{
    joo_set_int(g_pMethodsObj, name, (int)func);

    return 0;
}

int lt_methods_exec(const char * name, LT_RequestInfo * pInfo)
{
    serverProcessFunc func;

    pthread_mutex_lock(&mutex);
    
    func = (serverProcessFunc)joo_get_int(g_pMethodsObj, name);

    pthread_mutex_unlock(&mutex);

    if(func)
    {
        (*func)(pInfo);
    }
    else
    {
        joo_set_string(pInfo->replyData, "errcod", "ERR_NOENTRY");
        joo_set_string(pInfo->replyData, "errstr", "The request name is not exist.");
    }
    
    

    return 0;
}
