#include "stdio.h"
#include "stdlib.h"
#include "formLogin.h"
#include "lt_c_foundation.h"

/*******************************************
* global variable
*******************************************/
static LTMQHandle g_hMQ = NULL;

/******************************************
* interface function
******************************************/
boolean svr_init()
{
    lt_initial();
    g_hMQ = ltmq_init("127.0.0.1",0, 1);
    if(g_hMQ)
        return TRUE;

    return FALSE;
}

void svr_destroy()
{
    if(g_hMQ)
        ltmq_term(g_hMQ);

    g_hMQ = NULL;
}

LTMQHandle svr_GetHandle()
{
    return g_hMQ;
}
/******************************************
* service function
******************************************/

int svr_login(json_object * paramObj, json_object * resultObj)
{
    return 0;
}

