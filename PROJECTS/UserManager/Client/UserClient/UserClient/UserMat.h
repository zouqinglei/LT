/*
    formBuy.h
    write by zouql 20160403
    zouqinglei@163.com
*/

#ifndef USERCLIENT_USER_MAT_H
#define USERCLIENT_USER_MAT_H

#include "iup.h"
#include "iupcontrols.h"
#include "iupmatrixex.h"

#ifdef __cplusplus
extern "C" {
#endif


Ihandle * createUserMat();

void LoadUserData();

void SaveUserData();

void AppendRow();

void DeleteRow();




#ifdef __cplusplus
}
#endif

#endif /*USERCLIENT_USER_MAT_H*/