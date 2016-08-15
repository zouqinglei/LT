/*
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef USER_LOG_C_H
#define USER_LOG_C_H

#include "lt_commsocket.h"

typedef struct stLT_UserLog
{
	char logfile[MAX_PATH];

}LT_UserLog;

#ifdef __cplusplus
extern "C" {
#endif

void lt_userlog_init(LT_UserLog * pLog,const char * szLogFile);
void lt_userlog_destroy(LT_UserLog * pLog);

void lt_userlog(LT_UserLog * pLog,char *fmt,...); 

    
#ifdef __cplusplus
}
#endif

#endif /* USER_LOG_C_H */
