/* lt_record.h
 * write by zouql 20140823
 * simple record dataset package
 * wrapped for a record dataset based on json
 * zouqinglei@163.com 
    All right reserved. 
 */

#ifndef LT_RECORD_H
#define LT_RECORD_H

#include "json/json_short.h"

#ifdef  __cplusplus
extern "C" {
#endif 

/*
"status": 0: not modified, 1: modified, -1: new , -2: new modified
"mode": 0, server database build data, 1: client edit mode
"column type: string, int, 
recordset:
{
    "mode": 1,
    "column":[{"n":"a", "u": 1},{"n":"b", "u": 1},{"n":"c", "u": 1},{"n":"d", "u": 1}],
    "key": "xxx",
    "table": "updatetable",
    "data":[
         [],
         []
        ]

    "main":[
        {"s":0, "r": [{"s":0, "v": "xxx", "ov": "oldvalue"},{item},...]},
        ...
        ]

    "del":[
        {"s":0, "r": [{"s":0, "v": "xxx", "ov": "oldvalue"},{item},...]},
        ...
    ]


    
}
*/
json_object * ltr_new();
int ltr_del(json_object * handle);
int ltr_setmode(json_object * handle, int edit);
int ltr_setkey(json_object * handle, const char * key);
int ltr_settable(json_object * handle, const char * table);

int ltr_appendcol(json_object * handle,const char * colname, int updateable, int insertable);
int ltr_getcolidx(json_object * handle,const char * colname);
const char * ltr_getcolname(json_object * handle, int col);

int ltr_getcolumncount(json_object * handle);
int ltr_getrowcount(json_object * handle);

int ltr_insertrow(json_object * handle, int row);
int ltr_delrow(json_object * handle, int row);

int ltr_setitem(json_object * handle,int row, int col, const char * val);
int ltr_setitemN(json_object * handle,int row, const char * colname, const char * val);

const char * ltr_getitem(json_object * handle,int row, int col);
const char * ltr_getitemN(json_object * handle,int row, const char * colname);


int ltr_save(json_object * handle);

#ifdef __cplusplus
}
#endif

#endif /* LT_RECORD_H */
