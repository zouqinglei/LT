/*
    zouqinglei@163.com 
    All right reserved.
*/

#include "lt_userlog.h"


void lt_userlog_init(LT_UserLog * pLog,const char * szLogFile)
{
    int ret;
    ret = _snprintf(pLog->logfile,MAX_PATH - 1,"%s",szLogFile);
    if(ret < 0 || ret == (MAX_PATH - 1))
    {
        pLog->logfile[MAX_PATH - 1] = '\0';
    }
}

void lt_userlog_destroy(LT_UserLog * pLog)
{
}

void lt_userlog(LT_UserLog * pLog,char *fmt,...)
{

	char buf[1024 * 8];
    char szTimeStamp[128];
    char filename[1024];

    FILE *fd;
    struct tm  * ptm;
    time_t tim;

    va_list args;
	va_start(args,fmt);
	vsprintf(buf,fmt,args);
	va_end(args);

#ifdef _DEBUG
    printf("%s\n",buf);
#endif
	
    tim = time(NULL);
    ptm = localtime(&tim);

    _snprintf(szTimeStamp,127, "%4d-%02d-%02d",
            ptm->tm_year + 1900,
            ptm->tm_mon + 1,
            ptm->tm_mday);


    sprintf(filename,"%s%s.log",pLog->logfile,szTimeStamp);

    
	
	fd = fopen(filename,"a+");
	if(fd != NULL)
	{
        _snprintf(szTimeStamp,127, "%4d-%02d-%02d %02d:%02d:%02d",
            ptm->tm_year + 1900,
            ptm->tm_mon + 1,
            ptm->tm_mday,
            ptm->tm_hour,
            ptm->tm_min,
            ptm->tm_sec);

		fprintf(fd,"%s  %s\n",szTimeStamp,buf);
		fclose(fd);
	}


}


