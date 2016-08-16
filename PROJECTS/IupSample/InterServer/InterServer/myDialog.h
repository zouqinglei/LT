/*
    myToolbar.h
    write by zouql 20151019
    zouqinglei@163.com
*/

#ifndef INTERSERVER_MAIN_DIALOG_H
#define INTERSERVER_MAIN_DIALOG_H

#include "iup.h"
#include "iupim.h"
#include "iupcontrols.h"

#ifdef __cplusplus
extern "C" {
#endif


void createDialog();

void myMatrixAppend(unsigned int clientID, const char * clientIP, unsigned short port);

void myMatrixDelete(unsigned int clientID);



#ifdef __cplusplus
}
#endif

#endif /*INTERSERVER_MAIN_DIALOG_H*/