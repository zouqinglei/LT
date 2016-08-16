/*
  lt_ns_server.c
  write by zouql 20140826.
  zouqinglei@163.com


  1. use lua as server script language. 20141011

  2. update 20160317

*/

#include "lt_ns_server.h"
#include "lt_methods.h"
#include "lt_mq_queue.h"



/************************************************************
 *    global data declare
 ************************************************************/
extern char g_configfile[MAX_PATH];
extern char g_modpath[MAX_PATH];
extern LT_UserLog g_UserLog;
extern struct json_object * g_configObj;

struct stLTNSManager
{
    pthread_mutex_t mutex;
    LT_Thread * pProcessModuleThread; 

    struct json_object * configObj;
};

static struct stLTNSManager LTNSManager;

/***********************************************************
 * static variable and func declare
 ***********************************************************/


static void startModule(struct json_object * modObj)
{
    struct json_object * globalObj;
    struct json_object * curObj;
    struct json_object * procObj;
    int ret;
    char exepath[MAX_PATH];
    unsigned long procID;
    void * hProcess;
    void * hThread;

    unsigned short ltmqport;
    char * path;
    char * cmdline;
    int show;

    globalObj = joo_get(LTNSManager.configObj,"global");

    ltmqport = lt_config_getInt(globalObj, "serverport", 8100);

    path = lt_config_getString(modObj, "path", "");
    cmdline = lt_config_getString(modObj, "cmdline", "");
    show = lt_config_getInt(modObj, "show", 0);

#ifdef WIN32
    sprintf(exepath, "%s\\%s -p %d", 
        g_modpath, path, ltmqport);
#else
    sprintf(exepath, "%s/%s", svr_root, path);
#endif

    procID = 0;
    hProcess = NULL;
    hThread = NULL;
    ret = lt_forkexec(exepath,  cmdline, show, &procID, &hProcess, &hThread);
    if(ret == 0)
    {
        curObj = joo_get(modObj, "cur");
        procObj = jo_new_object();
        joo_set_int(procObj, "handle", (int)hProcess);
        joo_set_int(procObj, "thread", (int)hThread);
        joo_set_int(procObj, "pid", (int)procID);

        joa_add(curObj, procObj);
    }
      
   
}

/*
 "ECHO": {
                 "path":"server.exe",
                 "cmdline": "",
                 "min": 1,
                 "max": 1,  
                 "show": 0,
                 "statics":1, 
                 //dynamic
                 
                 "mode": {"NORMAL", "MAINTENANCE"},
                 "cur":[{"handle":xxx,"pid":xxx},{"handle":xxx,"pid":xxx}],
                 "svc":{"svc1": {"count": 10, 'max': 10, 'min': 1, 'avg': 5, 'tot':50}
                },

  }
*/
static void checkModule(struct json_object * modObj)
{
    int i,min, max, cur;
    struct json_object * curObj;
    const char * mode;

    curObj = joo_get(modObj, "cur");
    if(curObj == NULL)
    {
        curObj = jo_new_array();
        joo_add(modObj,"cur",curObj);
    }

    mode = joo_get_string(modObj, "mode");
    if(mode == NULL)
    {
        joo_set_string(modObj, "mode", "NORMAL"); 
        mode = joo_get_string(modObj, "mode");
    }

    if(strcmp(mode, "NORMAL") != 0)
        return;

    min = joo_get_int(modObj, "min");
    max = joo_get_int(modObj, "max");
    cur = joa_length(curObj);

    if(cur < min)
    {
        for(i = cur; i < min; i++)
        {
            startModule(modObj);
        }

    }
}

static void checkModules()
{
    struct json_object * modulesObj;

    modulesObj = joo_get(LTNSManager.configObj, "modules");
    if(modulesObj == NULL)
        return;
    
    pthread_mutex_lock(&LTNSManager.mutex);
    {
        json_object_object_foreach(modulesObj, key, val)
        {
            checkModule(val);        
        }
    }
    pthread_mutex_unlock(&LTNSManager.mutex);
}




#ifdef WIN32
static void clearModules(void * hProcess)
{
    int i,length;
    struct json_object * modulesObj;
    struct json_object * curObj;
    struct json_object * procObj;
    void * hProcess2;
    void * hThread;

    modulesObj = joo_get(LTNSManager.configObj, "modules");
    {
        json_object_object_foreach(modulesObj, key, modObj)
        {
            curObj = joo_get(modObj, "cur");  
            length = joa_length(curObj);
            for(i = 0; i < length; i++)
            {
                procObj = joa_get_idx(curObj, i);
                hProcess2 = (void *)joo_get_int(procObj,"handle");
                hThread = (void *)joo_get_int(procObj,"thread");
                if(hProcess2 == hProcess)
                {
                    CloseHandle(hProcess2);
                    CloseHandle(hThread);
                    joa_del_idx(curObj,i);
                    return;
                }
            }
        }
    }
}


void waitModules()
{
    unsigned int ret;
    HANDLE handles[128];
    int idx,count;
    int i,length;
    void * hProcess;
    
    struct json_object * modulesObj;
    struct json_object * curObj;
    struct json_object * procObj;

    modulesObj = joo_get(LTNSManager.configObj, "modules");
    if(modulesObj == NULL)
    {
        SLEEP(1);
        return;
    }

    idx = 0;

    pthread_mutex_lock(&LTNSManager.mutex);
    {
        json_object_object_foreach(modulesObj, key, modObj)
        {
            curObj = joo_get(modObj, "cur");  
            length = joa_length(curObj);
            for(i = 0; i < length; i++)
            {
                procObj = joa_get_idx(curObj, i);
                hProcess = (void *)joo_get_int(procObj,"handle");
                handles[idx++] = hProcess;
                if(idx == 126)
                    break;
            }

            if(idx == 126)
                break;
        }
    }
    pthread_mutex_unlock(&LTNSManager.mutex);
    
    count = idx;
    if(count == 0)
    {
        SLEEP(1);
        return;
    }

    ret = WaitForMultipleObjects(count, handles, FALSE, 5 * 1000);
    if((ret >= WAIT_OBJECT_0) &&  (ret < WAIT_OBJECT_0 + count))
    {
        idx = ret - WAIT_OBJECT_0;
        pthread_mutex_lock(&LTNSManager.mutex);
        clearModules(handles[idx]);
        pthread_mutex_unlock(&LTNSManager.mutex);

        /* second detech */
        while(idx < count)
        {
            ret = WaitForMultipleObjects(count - idx , &handles[idx], FALSE, 0);
            if(ret == WAIT_TIMEOUT)
                break;
            if(ret == WAIT_FAILED)
                break;

            idx = idx + (ret - WAIT_OBJECT_0);
            pthread_mutex_lock(&LTNSManager.mutex);
            clearModules(handles[idx]);
            pthread_mutex_unlock(&LTNSManager.mutex);
        }
    }
}

#else

static void clearModulesByPID(int pid)
{
    int i,length;
    struct json_object * modulesObj;
    struct json_object * curObj;
    struct json_object * procObj;
    int pid2;

    modulesObj = joo_get(LTNSManager.configObj, "modules");
    {
        json_object_object_foreach(modulesObj, key, modObj)
        {
            curObj = joo_get(modObj, "cur");  
            length = joa_length(curObj);
            for(i = 0; i < length; i++)
            {
                procObj = joa_get_idx(curObj, i);
                pid2 = (void *)joo_get_int(procObj,"pid");
                if(pid2 == pid)
                {
                    joa_del_idx(curObj,i);
                    return;
                }
            }
        }
    }
}

    /* linux or unix */

void signal_handler_SIGCHILD(int signal_num)
{
    pit_t pid;

    pid = waitpid(-1, NULL, WNOHANG);

    clearModulesByPID(pid);

}


#endif




/*
"ECHO": {
                 "path":"server.exe",
                 "cmdline": "",
                 "min": 1,
                 "max": 1,  
                 "show": 0,
                 "statics":1, 
                 //dynamic
                 
                 "mode": {"NORMAL", "MAINTENANCE"},
                 "cur":[{"handle":xxx,"pid":xxx},{"handle":xxx,"pid":xxx}],
                
                },



  */



/************************************************************************/
/* interface for as                                                  */
/************************************************************************/
pthread_handler_decl(handle_module,arg)
{
    LT_Thread * pThread;
	
    pThread = LTNSManager.pProcessModuleThread;

    while(!pThread->abort_loop)
    {
        //check and start user server
        checkModules();
#ifdef WIN32
        waitModules();
#else
        SLEEP(1);
#endif
    }

    LT_Thread_exit(pThread);
}


/**************************************************
 * service interface 
 *************************************************/
static void ltns_start(LT_RequestInfo * pInfo)
{
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * module_name;
    json_object * modulesObj;
    int bAll;
    int count;
    char errstr[128];

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    module_name  = joo_get_string(paramsObj, "module_name");

    count = 0;

    if(strcmp(module_name, "ALL") == 0)
    {
        //start all
        bAll = 1;
    }
    else
    {
        //start one
        bAll = 0;
    }
    
    modulesObj = joo_get(LTNSManager.configObj, "modules");
    {
        json_object_object_foreach(modulesObj, key, modObj)
        {
            if(bAll == 0)
            {
                if(strcmp(module_name, key) != 0)
                    continue;
            }
            //start it
            if(strcmp(joo_get_string(modObj, "mode"), "NORMAL") != 0)
            {
                joo_set_string(modObj, "mode", "NORMAL");
                count++;
            }
        }
    }
       
    joo_set_string(replyDataObj, "errcod","SUCCESS");
    sprintf(errstr, "start %d modules.", count);
    joo_set_string(replyDataObj, "errstr", errstr);
    
    return;
}
/*
    svc_ltns_start()
    requestData object:
    {
        "req":'NS.START',
        "params": {
            "module_name":"xxxx",
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":""
     }
*/
static void svc_ltns_start(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTNSManager.mutex);
    ltns_start(pInfo);
    pthread_mutex_unlock(&LTNSManager.mutex);
}


/*
call svc_ltmq_message_put
   
 
*/




static void manageServer_sendStopMsg(const char * modName)
{
    inter_mq_put("LTAS_USER_REQUEST", 
                         "SYS", 
                         modName, 
                         10, 
                         "STOP", 
                         NULL);
  
}

static void ltns_stopMod(const char * modName, struct json_object * modObj)
{
    struct json_object * curObj;
    int i,count;

    curObj = joo_get(modObj, "cur");  
    count = joa_length(curObj);

    joo_set_string(modObj, "mode", "MAINTENANCE");

    for(i = 0; i < count; i++)
    {
        //send stop msg to modName by LTAS_USER_REQUEST
        manageServer_sendStopMsg(modName);
    }
}


static void ltns_stop(LT_RequestInfo * pInfo)
{
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * module_name;

    json_object * modulesObj;
    int bAll;
    int count;
    char errstr[128];

    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    module_name  = joo_get_string(paramsObj, "module_name");
    count = 0;

    if(strcmp(module_name, "ALL") == 0)
    {
        //stop all
        bAll = 1;
    }
    else
    {
        //stop one
        bAll = 0;
    }
    
    modulesObj = joo_get(LTNSManager.configObj, "modules");
    {
        json_object_object_foreach(modulesObj, key, modObj)
        {
            if(bAll == 0)
            {
                if(strcmp(module_name, key) != 0)
                    continue;
            }
            //stop it
            if(strcmp(joo_get_string(modObj, "mode"), "NORMAL") == 0)
            {
                joo_set_string(modObj, "mode", "MAINTENANCE");
                //send stop message to LTNS_USER_REQUEST 
                ltns_stopMod(key, modObj);
                count++;
            }
        }
    }
       
    joo_set_string(replyDataObj, "errcod","SUCCESS");
    sprintf(errstr, "stopping %d modules.");
    joo_set_string(replyDataObj, "errstr", errstr);
    
    return;
}
/*
    svc_ltns_stop()
    requestData object:
    {
        "req":'NS.STOP',
        "params": {
            "module_name":"xxxx",
        }
     }

     replyData object:
     {
        "errcod":"SUCCESS"
        "errstr":""
     }
*/
static void svc_ltns_stop(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTNSManager.mutex);
    ltns_stop(pInfo);
    pthread_mutex_unlock(&LTNSManager.mutex);
}


/*
"ECHO": {
                 "path":"server.exe",
                 "cmdline": "",
                 "min": 1,
                 "max": 1,  
                 "show": 0,
                 
                 //dynamic
                 
                 "mode": {"NORMAL", "MAINTENANCE"},
                 "cur":[{"handle":xxx,"pid":xxx},{"handle":xxx,"pid":xxx}],
 }
*/
static void ltns_info(LT_RequestInfo * pInfo)
{
    struct json_object * modulesObj;
    json_object * paramsObj;
    json_object * replyDataObj;
    const char * queue_name;
    json_object * hRecord;
    int row;
    int count;
    char tmp[32];


    paramsObj = joo_get(pInfo->requestData, "params");
    replyDataObj = (json_object *)pInfo->replyData;

    queue_name = joo_get_string(paramsObj, "name");

    hRecord = ltr_new();
    ltr_appendcol(hRecord, "NAME", 1, 1);
    ltr_appendcol(hRecord, "PATH", 1, 1);
    ltr_appendcol(hRecord, "CMDLINE", 1, 1);
    ltr_appendcol(hRecord, "SHOW", 1, 1);
    ltr_appendcol(hRecord, "MIN", 1, 1);
    ltr_appendcol(hRecord, "MAX", 1, 1);
    ltr_appendcol(hRecord, "CUR", 1, 1);
    ltr_appendcol(hRecord, "STATUS", 1, 1);
    
    modulesObj = joo_get(LTNSManager.configObj, "modules");
    if(modulesObj == NULL)
    {
        return;
    }

    {
        json_object_object_foreach(modulesObj, key, modObj)
        {
            row = ltr_insertrow(hRecord,-1);
            ltr_setitem(hRecord, row, 0, key);
            ltr_setitem(hRecord, row, 1, joo_get_string(modObj, "path"));
            ltr_setitem(hRecord, row, 2, joo_get_string(modObj, "cmdline"));
            ltr_setitem(hRecord, row, 3, joo_get_string(modObj, "show"));
            ltr_setitem(hRecord, row, 4, joo_get_string(modObj, "min"));
            ltr_setitem(hRecord, row, 5, joo_get_string(modObj, "max"));
            count = joa_length(joo_get(modObj, "cur"));
            sprintf(tmp,"%d",count);
            ltr_setitem(hRecord, row, 6, tmp);
            ltr_setitem(hRecord, row, 7, joo_get_string(modObj, "mode"));

        }
    }

    joo_set_string(replyDataObj, "errcod", "SUCCESS");
    joo_set_string(replyDataObj, "errstr", "");
    
    joo_add(replyDataObj, "data", hRecord);

}

static void svc_ltns_info(LT_RequestInfo * pInfo)
{
    pthread_mutex_lock(&LTNSManager.mutex);
    ltns_info(pInfo);
    pthread_mutex_unlock(&LTNSManager.mutex);
}


static int init_methods()
{
    lt_methods_register( "NS.START", svc_ltns_start);
    lt_methods_register( "NS.STOP", svc_ltns_stop);
    lt_methods_register( "NS.INFO", svc_ltns_info);
 
    return 0;
}

/********************************************
 * global func
 *******************************************/

int ltns_init()
{
    lt_userlog(&g_UserLog, "init lt name server...");
    pthread_mutex_init(&LTNSManager.mutex, NULL);

    LTNSManager.configObj = g_configObj;
   
    init_methods();

#ifdef WIN32

#else
    signal(SIGINT, signal_handler_SIG_CHILD);
#endif

    /* start process thread */
    LTNSManager.pProcessModuleThread = LT_Thread_new(handle_module, NULL);
   
    lt_userlog(&g_UserLog, "start lt app server...");
    LT_Thread_start(LTNSManager.pProcessModuleThread);
  
    return 0;
}



int ltns_destroy()
{
    lt_userlog(&g_UserLog, "stop lt app server...");

    LT_Thread_stop(LTNSManager.pProcessModuleThread);
  
    /* stop proces thread */
    LT_Thread_free(LTNSManager.pProcessModuleThread);
  
    pthread_mutex_destroy(&LTNSManager.mutex);

    //lt_userlog(&g_UserLog, "exit lt app server");

    return 0;
}


