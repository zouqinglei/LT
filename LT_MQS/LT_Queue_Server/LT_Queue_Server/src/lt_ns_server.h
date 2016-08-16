/*
  lt_as_server.h
  write by zouql 20140826.
  zouqinglei@163.com
  all right reserved.

  lt name server package

     
*/

#ifndef LT_NS_SERVER_H__
#define LT_NS_SERVER_H__

#include "lt_c_foundation.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
{
    "global": 
    {
        "serverport":8100,
		"maxclient":100,
		"maxworkthreads":10,
    },

    "modules": 
    {
       "ECHO": {
                 "path":"server.exe",
                 "cmdline": "",
                 "min": 1,
                 "max": 1,  
                 "cur":[{"handle":xxx,"pid":xxx},{"handle":xxx,"pid":xxx}]
                },
        "Yard": {
                 "path":"yard.exe",
                 "cmdline": "",
                 "min": 1,
                 "max": 5, 
                },
     }
}
*/




/************************************************************************/
/* interface for as                                                     */
/************************************************************************/ 
int ltns_init();

int ltns_destroy();




#ifdef __cplusplus
}
#endif


#endif /*LT_AS_SERVER_H__*/


