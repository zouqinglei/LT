#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lt_mqs.h"
#include "lt_c_foundation.h"


/**************************************************
 * global variable
 *************************************************/

sqlite3 * g_db = NULL;

/*************************************************
 * local function 
 ************************************************/

static boolean ConnectDB()
{
    int result;
    char db_path[MAX_PATH];
    char * errmsg;

    lt_GetModulePath(db_path, MAX_PATH);
    strcat(db_path, "\\..\\..\\user.db");
    result = sqlite3_open(db_path, &g_db);

    if(result != SQLITE_OK)
        return FALSE;

    //multi process access timeout
    sqlite3_exec(g_db, "PRAGMA busy_timeout=10000", 0, 0, &errmsg);

    return TRUE;
}


static boolean DisconnectDB()
{
    sqlite3_close(g_db);

    return TRUE;
}
/***************************************************
* module service
***************************************************/

static void login(LtSvcInfo * pInfo)
{
    const char * username;
    const char * password;
    char sql[1024];
    char md5[33];
    sqlite3_stmt * stmt;
    int result;
    int count;

    username = joo_get_string(pInfo->paramObj, "username");
    password = joo_get_string(pInfo->paramObj, "password");
    
    //check username and password
    lt_GenMD5(password, md5, 33);
    sprintf(sql, "SELECT COUNT(*) FROM USERACCOUNT \
                 WHERE USERNAME = '%s' AND PASSWORD = '%s'",
                 username,
                 md5);

    count = 0;
    result = sqlite3_prepare(g_db, sql, -1, &stmt, NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        if(result == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    if(count == 1)
    {
        joo_set_string(pInfo->resultObj, "errcod", "SUCCESS");
        joo_set_string(pInfo->resultObj, "errstr", "user login ok.");
    }
    else
    {
        joo_set_string(pInfo->resultObj, "errcod", "ERR_NOENT");
        joo_set_string(pInfo->resultObj, "errstr", "username or password error.");
    }

    return;
}

static void logout(LtSvcInfo * pInfo)
{
    const char * username;

    username = joo_get_string(pInfo->paramObj, "username");

    //process logout

    joo_set_string(pInfo->resultObj, "errcod", "SUCCESS");
    joo_set_string(pInfo->resultObj, "errstr", "user logout ok.");

    return;
}

static void loadData(LtSvcInfo * pInfo)
{
    char sql[1024];
    sqlite3_stmt * stmt;
    int result;
    json_object * ltr;
    int row;

    ltr = ltr_new();
    ltr_appendcol(ltr, "UID", 1,1);
    ltr_appendcol(ltr, "USERNAME", 1,1);
    ltr_appendcol(ltr, "PASSWORD", 1,1);
    ltr_appendcol(ltr, "REMARK", 1,1);

    ltr_setkey(ltr, "UID");
    ltr_settable(ltr, "USERACCOUNT");

    sprintf(sql, "SELECT UID, USERNAME, PASSWORD, REMARK FROM USERACCOUNT");

    result = sqlite3_prepare(g_db, sql, -1, &stmt, NULL);
    if(result == SQLITE_OK)
    {
        result = sqlite3_step(stmt);
        while(result == SQLITE_ROW)
        {
            row = ltr_insertrow(ltr,-1);
            ltr_setitem(ltr, row, 0, sqlite3_column_text(stmt, 0));
            ltr_setitem(ltr, row, 1, sqlite3_column_text(stmt, 1));
            ltr_setitem(ltr, row, 2, sqlite3_column_text(stmt, 2));
            ltr_setitem(ltr, row, 3, sqlite3_column_text(stmt, 3));

            result = sqlite3_step(stmt);
        }

        joo_set_string(pInfo->resultObj, "errcod", "SUCCESS");
        joo_set_string(pInfo->resultObj, "errstr", "");
        
        joo_add(pInfo->resultObj, "data", ltr);
    }
    else
    {
        joo_set_string(pInfo->resultObj, "errcod", "ERR_SQL");
        joo_set_string(pInfo->resultObj, "errstr", "DB error.");
    }

    sqlite3_finalize(stmt);

}

static void saveData(LtSvcInfo * pInfo)
{
    int result;
    const char * sql;
    char * errmsg;

    sql = joo_get_string(pInfo->paramObj, "sql");

    result = sqlite3_exec(g_db, sql, NULL, NULL, &errmsg);

    if(result == SQLITE_OK)
    {
        joo_set_string(pInfo->resultObj, "errcod", "SUCCESS");
        joo_set_string(pInfo->resultObj, "errstr", "");
        
    }
    else
    {
        joo_set_string(pInfo->resultObj, "errcod", "ERR_SQL");
        joo_set_string(pInfo->resultObj, "errstr", errmsg);
    }

}

int main(int argc, char *argv[])
{
    printf("Welcome USER module.\n");
    //init area
    //connect sqlite3 db supplychain.db
    if(ConnectDB() == FALSE)
        return -1;

    ltns_register("LOGIN", login);
    ltns_register("LOGOUT", logout);
    ltns_register("LOADDATA", loadData);
    ltns_register("SAVE", saveData);

    ltns_serverloop("USER", argc, argv);

    //destroy area
    DisconnectDB();

    return 0;
}
