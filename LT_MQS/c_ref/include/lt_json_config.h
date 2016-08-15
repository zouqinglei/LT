/*
  modulemanager.h: write by zouql 20120508.
  1. module manager for exe application server

  module and service registry
    
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef HDM2_CONFIG_H__
#define HDM2_CONFIG_H__


#include "json/json_short.h"


#ifdef __cplusplus
extern "C" {
#endif

struct json_object *  lt_config_init(char * configfile);
int lt_config_destroy(struct json_object * configObj);

char * lt_config_getString(struct json_object * obj, char * key, char * defaultValue);
int  lt_config_getInt(struct json_object * obj, char * key, int defaultValue);

#ifdef __cplusplus
}
#endif


#endif /*HDM2_CONFIG_H__*/


