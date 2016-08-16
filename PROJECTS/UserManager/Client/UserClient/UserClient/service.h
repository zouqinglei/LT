/*
    formBuy.h
    write by zouql 20160403
    zouqinglei@163.com
*/

#ifndef SUPPLYCHAIN_SERVICE_H
#define SUPPLYCHAIN_SERVICE_H

#include "iup.h"
#include "iupcontrols.h"

#include "lt_mqs.h"

#ifdef __cplusplus
extern "C" {
#endif

boolean svr_init();
void svr_destroy();
LTMQHandle svr_GetHandle();

int svr_login(json_object * paramObj, json_object * resultObj);




#ifdef __cplusplus
}
#endif

#endif /*SUPPLYCHAIN_SERVICE_H*/