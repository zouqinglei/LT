/*
  lt_methods.h
  write by zouql 20160217.
  zouqinglei@163.com
  all right reserved.
     
*/

#ifndef LT_METHODS_H__
#define LT_METHODS_H__

#include "lt_c_foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* interface for methods                                                    */
/************************************************************************/ 
int lt_methods_init();

int lt_methods_destroy();

int lt_methods_register(const char * name, serverProcessFunc func);
int lt_methods_exec(const char * name, LT_RequestInfo * pInfo);


#ifdef __cplusplus
}
#endif


#endif /*LT_METHODS_H__*/


