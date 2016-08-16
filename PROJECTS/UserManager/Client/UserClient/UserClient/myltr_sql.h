/*
    myltr_sql.h
    write by zouql 20160411
    zouqinglei@163.com
    All rights reserved.
*/

#ifndef MYLTR_SQLITE3_H
#define MYLTR_SQLITE3_H

#include "lt_c_foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

const char * sqlite3_getsql(json_object * handle);



#ifdef __cplusplus
}
#endif

#endif /*MYLTR_SQLITE3_H*/