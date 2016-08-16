#include "stdio.h"
#include "stdlib.h"
#include "UserMat.h"
#include "service.h"
#include "lt_c_foundation.h"
#include "myltr_sql.h"

static Ihandle * mat;
static json_object * g_record = NULL;
static int cur_lin, cur_col;

static char * on_value_cb(Ihandle * ih, int lin, int col)
{
    if(g_record == NULL)
        return "";

    if(lin == 0)
    {
        return (char *)ltr_getcolname(g_record, col - 1);
    }
    
    if(col == 0)
        return "";

    return (char *)ltr_getitem(g_record, lin - 1, col - 1);
}

static  int on_value_edit_cb(Ihandle * ih, int lin, int col, char * newval)
{
    char md5[33];

    if(col == 3) //convert to md5
    {
        lt_GenMD5(newval, md5, 33);
        ltr_setitem(g_record, lin -1, col - 1, md5);
    }
    else
    {
        ltr_setitem(g_record, lin -1, col - 1, newval);
    }

    return IUP_DEFAULT;
}

static int on_enterItem_cb(Ihandle * ih, int lin, int col)
{
    cur_lin = lin;
    cur_col = col;
    
    return IUP_DEFAULT;
}


Ihandle * createUserMat()
{
    mat = IupMatrixEx();
    IupSetAttribute(mat, "NAME", "matrix");
    IupSetAttribute(mat, "EXPAND", "Yes");
    IupSetAttribute(mat, "RESIZEMATRIX", "YES");
    IupSetAttribute(mat, "SCROLLBAR", "YES");
    IupSetAttribute(mat, "BACKGROUND ", "200 10 80");
    
    IupSetAttribute(mat, "NUMCOL", "4");
    IupSetAttribute(mat, "NUMLIN", "0");

    IupSetAttribute(mat,"NUMCOL_VISIBLE","3");
    IupSetAttribute(mat,"NUMLIN_VISIBLE","3");

    /*
    IupSetAttribute(mat, "0:0", "SEQ");
    IupSetAttribute(mat, "0:1", "UID");
    IupSetAttribute(mat, "0:2", "USERNAME");
    IupSetAttribute(mat, "0:3", "PASSWORD");
    IupSetAttribute(mat, "0:4", "REMARK");
    */
    IupSetAttribute(mat, "HEIGHT0", "10");
    

    IupSetCallback(mat, "VALUE_CB", (Icallback)on_value_cb);
    IupSetCallback(mat, "VALUE_EDIT_CB", (Icallback)on_value_edit_cb);
    IupSetCallback(mat, "ENTERITEM_CB", (Icallback)on_enterItem_cb);
    

    return mat;
}

static void FillMatrix(json_object * ltr)
{
    


}

void LoadUserData()
{
    int ret;
    json_object * paramObj;
    json_object * resultObj;
    const char * errcod;
    const char * errstr;
    int rowcount;
    const char * datastr;

    paramObj = jo_new_object();
    resultObj = jo_new_object();

    ret = ltns_call(svr_GetHandle(), "USER", "LOADDATA", paramObj, resultObj, 10);
    if(ret == 0)
    {
        errcod = joo_get_string(resultObj, "errcod");
        errstr = joo_get_string(resultObj, "errstr");
        if(strcmp(errcod, "SUCCESS") == 0)
        {
            if(g_record)
            {
                jo_put(g_record);
            }
            g_record = joo_get(resultObj,"data");
            datastr = jo_to_json_string(g_record);
            jo_get(g_record);
            rowcount = ltr_getrowcount(g_record);
            ltr_setmode(g_record, 1);
            IupSetInt(mat, "NUMLIN", rowcount);
            IupSetAttribute(mat, "REDRAW","ALL");
            IupRefresh(mat);

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

void SaveUserData()
{
    const char * sql;
    int ret;
    json_object * paramObj;
    json_object * resultObj;
    const char * errcod;
    const char * errstr;

    sql = sqlite3_getsql(g_record);

    if( strlen(sql) == 0)
        return;

    paramObj = jo_new_object();
    resultObj = jo_new_object();

    joo_set_string(paramObj, "sql", sql);
    ret = ltns_call(svr_GetHandle(), "USER", "SAVE", paramObj, resultObj, 10);
    if(ret == 0)
    {
        errcod = joo_get_string(resultObj, "errcod");
        errstr = joo_get_string(resultObj, "errstr");
        if(strcmp(errcod, "SUCCESS") == 0)
        {
            IupMessage("Warning", "Save Success.");
            ltr_save(g_record);
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
}


void AppendRow()
{
    int row;
    int rowcount;
    char uuid[40];

    lt_GenUUID(uuid,40);

    rowcount = ltr_getrowcount(g_record);
    row = ltr_insertrow(g_record, -1);
    ltr_setitem(g_record, row, 0, uuid);

    IupSetInt(mat, "ADDLIN", rowcount); 
}

void DeleteRow()
{
    if( ltr_delrow(g_record, cur_lin - 1) == 0)
    {
        IupSetInt(mat, "DELIN", cur_lin);
        IupSetInt(mat, "NUMLIN", ltr_getrowcount(g_record));
    }
}