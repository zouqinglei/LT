/*
   lt_json_config.c 
   write by zouqinglei@163.com 20150415
       
    zouqinglei@163.com 
    All right reserved.

*/
#include "lt_json_config.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

/************************************************************
 *    global data declare
 ************************************************************/

struct json_object * configObj;

/***********************************************************
 * static variable and func declare
 ***********************************************************/


/***********************************************************
 *  func 
 ***********************************************************/
struct json_object * lt_config_init(char * configfile)
{
    struct json_object * configObj;
    char * pbuf;
    char * ptr;
    FILE * fp;

    struct _stat fst;
    int ret;

    ret = _stat(configfile,&fst);

    if(ret != 0)
    {
        //userlog
        return NULL;
    }

    fp = fopen(configfile,"r");
    if(!fp)
    {
        //userlog
        return NULL;
    }

    pbuf = (char *)malloc(fst.st_size + 1);
    
    ptr = pbuf;
    while((ret = fread(ptr,1,min(1024,fst.st_size),fp)) > 0)
    {
        ptr += ret;
    }

    fclose(fp);

    *ptr = '\0';

    while(ptr > pbuf)
    {
        if(*ptr == '\r' || *ptr == '\n')
        {
            *ptr = ' ';
        }
        ptr--;
    }

    configObj = json_tokener_parse(pbuf);
    free(pbuf);
    if(is_error(configObj))
        return NULL;
   
    return configObj;
}

int lt_config_destroy(struct json_object * configObj)
{
    jo_put(configObj);
    return 0;
}

char * lt_config_getString(struct json_object * obj, char * key, char * defaultValue)
{
    struct json_object * tmpobj;
    tmpobj = joo_get(obj,key);
    if(tmpobj == NULL)
        return defaultValue;

    return (char *)jo_get_string(tmpobj);
}

int  lt_config_getInt(struct json_object * obj, char * key, int defaultValue)
{
    struct json_object * tmpobj;
    tmpobj = joo_get(obj,key);
    if(tmpobj == NULL)
        return defaultValue;

    return jo_get_int(tmpobj);
}
