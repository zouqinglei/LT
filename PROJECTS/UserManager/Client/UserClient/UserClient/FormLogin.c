#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "formLogin.h"
#include "service.h"

boolean DlgLogin()
{
    int ret;
    boolean bRet;
    json_object * paramObj;
    json_object * resultObj;
    const char * errcod;
    const char * errstr;
    
    char username[128] = "";
    char password[128] = "";

    bRet = FALSE;


    ret = IupGetParam("Login", NULL, 0,
        "Bt %u[,Cancel] \n"
        "UserName: %s\n"
        "PassWord: %s\n",
        username, password);

    if(ret == 1)
    {
        paramObj = jo_new_object();
        resultObj = jo_new_object();

        joo_set_string(paramObj, "username", username);
        joo_set_string(paramObj, "password", password);

        ret = ltns_call(svr_GetHandle(), "USER", "LOGIN",  paramObj, resultObj, 10);

        if(ret == 0)
        {
            errcod = joo_get_string(resultObj, "errcod");
            errstr = joo_get_string(resultObj, "errstr");
            if(strcmp(errcod, "SUCCESS") == 0)
            {
                IupMessage("Warning", "Login Success");
                bRet = TRUE;

            }
            else
            {
                IupMessage("Warning", errstr);
            }
        }
        else
        {
            IupMessagef("Warning", "An error occured: %d", ret);
        }

        jo_put(paramObj);
        jo_put(resultObj);
    }

    return bRet;
}