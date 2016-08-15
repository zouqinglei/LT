/*
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef LT_UTILS_H
#define LT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


char * lt_dirname(char *path);
char * lt_basename(char *path, char *suffix);

int lt_makeargv(const char *s, const char *delimiters, char ***argvp);
void lt_freemakeargv(char **argv);

int lt_GenUUID(char *uuid,int size);

int lt_forkexec(char * exec_path_name,
                   char * cmd_line, 
                   int showWindow, 
                   unsigned long * procID,
                   void ** phProcess,
                   void ** phThread);

unsigned long lt_getpid();

unsigned long lt_getTickCount();

int lt_GenMD5(const char * str, char * md5, int size);

const char * lt_GetModulePath(char * modulePath, int size);

#ifdef __cplusplus
}
#endif

#endif
