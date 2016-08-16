/*
    myMenu.h
    write by zouql 20151019
    zouqinglei@163.com
*/

#ifndef INTERSERVER_MENU_H
#define INTERSERVER_MENU_H

#include "iup.h"

#ifdef __cplusplus
extern "C" {
#endif


void createMenu();
Ihandle * createToolbar();
Ihandle * createStatusbar();






#ifdef __cplusplus
}
#endif

#endif /*INTERSERVER_MENU_H*/