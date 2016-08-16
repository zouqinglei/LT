/*
    myRes.h
    write by zouql 20151019
    zouqinglei@163.com
*/

#ifndef INTERSERVER_MENU_H
#define INTERSERVER_MENU_H

#include "iup.h"
#include "iupcontrols.h"

#ifdef __cplusplus
extern "C" {
#endif


void createMenu();
Ihandle * createToolbar();
Ihandle * createStatusbar();
Ihandle * createMatrix();


#ifdef __cplusplus
}
#endif

#endif /*INTERSERVER_MENU_H*/