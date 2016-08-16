/************************************************************************
* printtable.h
* write by zouqinglei@163.com 20141024
************************************************************************/
#ifndef LT_PRINT_TABLE_H_
#define LT_PRINT_TABLE_H_

#include "lt_c_foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
recordset:
{"column":["a","b","c","d"],
 "data":[
         [  ],
         [  ]
        ]
}
*/
int  print_table(json_object * hRecord);

#ifdef __cplusplus
}
#endif

#endif/*ECP_TABLE_H_*/
