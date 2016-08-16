/*
    lt_queue main.c
    write by zouql 20140723
    zouqinglei@163.com
*/

#include "lt_c_foundation.h"
#include "lt_mq_server.h"
#include "lt_mq_queue.h"
#include "lt_methods.h"
#include "lt_ns_server.h"


char  SERVICENAME[256] = "LTMQS";
char  SERVICEDISPLAYNAME[256] = "LT Msg Queue Server v1.0";

char g_work_path[MAX_PATH];
char g_configfile[MAX_PATH];
char g_logfile[MAX_PATH];
char g_dbfile[MAX_PATH];
char g_modpath[MAX_PATH];

LT_UserLog g_UserLog;
struct json_object * g_configObj;

LT_ServerParam g_server_param;

/* wait end */
pthread_mutex_t wait_mutex;
pthread_cond_t  wait_cond;


#ifdef __WIN__
/*extern void _CRTAPI1 win_main(int argc, char **argv);*/
extern void  win_main(int argc, char **argv);
extern BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
#endif

int abort_loop;

int server_init()
{
    
    /* default config file */
#ifdef WIN32
    char path[MAX_PATH];
    GetModuleFileName(NULL,path,MAX_PATH);
    sprintf(g_work_path,"%s",lt_dirname(path));

    sprintf(g_configfile,"%s\\%s", g_work_path, "ltmqs.conf");
    sprintf(g_logfile,"%s\\log\\%s", g_work_path, "ltmqs");
    sprintf(g_dbfile,"%s\\%s", g_work_path, "ltmqs.db");
    sprintf(g_modpath,"%s\\modules",g_work_path);
    
#else
    sprintf(g_configfile,"%s/%s",getenv("LTMQ_PATH"),"ltmqs.conf");
    sprintf(g_logfile,"%s/%s",getenv("LTMQ_PATH"),"ltmqs.log");
    sprintf(g_dbfile,"%s/%s",getenv("LTMQ_PATH"),"ltmqs.db");
#endif

    lt_userlog_init(&g_UserLog,g_logfile);
 
    lt_userlog(&g_UserLog, "ltmqs initialize...\n");

    g_configObj = lt_config_init(g_configfile);
	if(!g_configObj)
    {
        lt_userlog(&g_UserLog, "load config file %s error.\n",g_configfile);
        return LT_ERR_FAIL;
    }
  
    lt_userlog(&g_UserLog, "load config file %s successful.\n",g_configfile);


#ifndef __WIN__
    signal(SIGPIPE,SIG_IGN);
#endif

    lt_initial();

    lt_methods_init();

    ltmq_queue_init();

    ltmq_server_init();

    ltns_init();
  
    return 0;
}


int server_destroy()
{
    /* service clean ,queue module*/

    ltns_destroy();

    ltmq_server_destroy();

    ltmq_queue_destroy();

    lt_methods_destroy();

    lt_config_destroy(g_configObj);

    lt_userlog(&g_UserLog, "ltmqs quit.\n");

    return 0;
}


void ServiceStart(int argc, char **argv)
{  
    struct timespec  ts;

    if(server_init() < 0)
       return;

    
    
#ifdef  __WIN__
        ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0 );
#endif   

    /*wait stop */
    abort_loop = 0;
    ts.tv_sec = 2;
    ts.tv_nsec = 0;
    pthread_mutex_init(&wait_mutex,NULL);
    pthread_cond_init(&wait_cond,NULL);

    pthread_mutex_lock(&wait_mutex);
    while(!abort_loop)
    {
        my_pthread_cond_timedwait(&wait_cond, &wait_mutex, &ts);
    }
    pthread_mutex_unlock(&wait_mutex);
    
    /* server end cleanup */
    server_destroy();
    
   
}

void ServiceStop()
{
    abort_loop = 1;
	pthread_cond_signal(&wait_cond);
#ifdef __WIN__
    ReportStatusToSCMgr( SERVICE_STOPPED, NO_ERROR, 0 );
#endif

}

int main(int argc, char *argv[])
{
    
#ifdef  __WIN__
    win_main(argc,argv);
#else

/* linux */
    e_todaemon();
    ServiceStart(argc,argv);
#endif
    
    return 0;
}

