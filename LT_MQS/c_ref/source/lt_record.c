#include "lt_record.h"
#include "lt_c_foundation.h"


/*
recordset:
{"column":["a","b","c","d"],
 "data":[
         [  ],
         [  ]
        ]
}
*/

/**********************************
 * function declare
 *********************************/
static int ltr_insertrowEdit(json_object * handle, int row);
static int ltr_delrowEdit(json_object * handle, int row);
static int ltr_getrowcountEdit(json_object * handle);
static const char * ltr_getitemEdit(json_object * handle,int row, int col);
static int ltr_setitemEdit(json_object * handle,int row, int col, const char * val);
/**********************************
 * interface 
 ********************************/
json_object * ltr_new()
{
    json_object * record;

    record = jo_new_object();
    joo_add(record, "column",jo_new_array());
    joo_add(record, "data",jo_new_array());

    return record;

}

int ltr_del(json_object * handle)
{
    if(handle)
    {
        jo_put(handle);
    }
    return 0;
}


int ltr_appendcol(json_object * handle,const char * colname, int updateable, int insertable)
{
    json_object * columnArray;
    json_object * columnObj;
    
    if(!handle || !colname)
        return LT_ERR_PARAM;
    
    columnObj = jo_new_object();
    joo_set_string(columnObj, "n", colname);
    joo_set_int(columnObj, "u", updateable);
    joo_set_int(columnObj, "i", insertable);

    columnArray = joo_get(handle, "column");
    joa_add(columnArray, columnObj);
    
    return joa_length(columnArray) - 1;
}

int ltr_getcolidx(json_object * handle,const char * colname)
{
    int i;
    json_object * columnArray;
    json_object * columnObj;
    int columncount;
    const char * tmp;
    
    if(!handle || !colname)
        return -1;
    
    columnArray = joo_get(handle, "column");

    columncount = joa_length(columnArray);

    for(i = 0; i < columncount; i++)
    {
        columnObj = joa_get_idx(columnArray,i);
        tmp = joo_get_string(columnObj,"n");
        if(strcmp(colname, tmp) == 0)
        {
            return i;
        }
    }

    return -1;
}

const char * ltr_getcolname(json_object * handle, int col)
{
    struct json_object * columnArray;
    int columncount;

    if(!handle )
        return NULL;
    
    columnArray = joo_get(handle, "column");

    columncount = joa_length(columnArray);

    if(col < 0 || col >= columncount)
        return NULL;

    return joo_get_string(joa_get_idx(columnArray, col), "n");
}

int ltr_getcolumncount(json_object * handle)
{
    struct json_object * columnArray;
    
    if(!handle )
        return 0;
    
    columnArray = joo_get(handle, "column");

    return joa_length(columnArray);
}

int ltr_getrowcount(json_object * handle)
{
    struct json_object * dataobj;

    if(!handle )
        return LT_ERR_PARAM;

    if(joo_get_int(handle, "mode") == 1)
    {
        return ltr_getrowcountEdit(handle);
    }

    dataobj = joo_get(handle, "data");

    return joa_length(dataobj);
}

int ltr_insertrow(json_object * handle, int row)
{
    int i;
    struct json_object * columnArray;
    struct json_object * dataobj;
    struct json_object * rowobj;
    int columncount;
    int rowcount;

    if(!handle )
        return LT_ERR_PARAM;

    if(joo_get_int(handle, "mode") == 1)
    {
        return ltr_insertrowEdit(handle, row);
    }
    
    columnArray = joo_get(handle, "column");
    dataobj = joo_get(handle, "data");

    columncount = joa_length(columnArray);
    rowcount = joa_length(dataobj);
    if(row < 0 || row >= rowcount)
    {
        row= rowcount;
    }

    rowobj = jo_new_array();
    for(i = 0; i < columncount; i++)
    {
        joa_add(rowobj, jo_new_string(""));
    }

    joa_insert_idx(dataobj, row, rowobj);

    return row;
}

int ltr_delrow(json_object * handle, int row)
{
    struct json_object * dataobj;
    int rowcount;

    if(!handle )
        return LT_ERR_PARAM;
    
    if(joo_get_int(handle, "mode") == 1)
    {
        return ltr_delrowEdit(handle, row);
    }

    dataobj = joo_get(handle, "data");

    rowcount = joa_length(dataobj);
    if(row < 0 || row >= rowcount)
    {
        return LT_ERR_PARAM;
    }
   
    joa_del_idx(dataobj, row);
    
    return 0;
}

int ltr_setitem(json_object * handle,int row, int col, const char * val)
{
    json_object * columnArray;
    json_object * dataArray;
    json_object * rowObj;
    int columncount;
    int rowcount;

    if(!handle || !val)
        return LT_ERR_PARAM;

    

    if(joo_get_int(handle, "mode") == 1)
    {
        return ltr_setitemEdit(handle, row, col, val);
    }

    columnArray = joo_get(handle, "column");
    columncount = joa_length(columnArray);
    if(col < 0 || col >= columncount)
    {
        return LT_ERR_PARAM;
    }
    
    dataArray = joo_get(handle, "data");
    rowcount = joa_length(dataArray);
    if(row < 0 || row >= rowcount)
    {
        return LT_ERR_PARAM;
    }

    rowObj = joa_get_idx(dataArray, row);
    joa_put_idx(rowObj, col, jo_new_string(val));

    return 0;
}

int ltr_setitemN(json_object * handle,int row, const char * colname, const char * val)
{
    int col;
    col = ltr_getcolidx(handle, colname);

    return ltr_setitem(handle,row,col,val);
}

const char * ltr_getitem(json_object * handle,int row, int col)
{
    json_object * columnArray;
    json_object * dataArray;
    json_object * rowObj;
    int columncount;
    int rowcount;

    if(!handle)
        return NULL;

    if(joo_get_int(handle, "mode") == 1)
    {
        return ltr_getitemEdit(handle, row, col);
    }
    
    columnArray = joo_get(handle, "column");
    dataArray = joo_get(handle, "data");

    columncount = joa_length(columnArray);
    rowcount = joa_length(dataArray);
    if(row < 0 || row >= rowcount)
    {
        return NULL;
    }

    if(col < 0 || col >= columncount)
    {
        return NULL;
    }

    rowObj = joa_get_idx(dataArray, row);

    return jo_get_string(joa_get_idx(rowObj, col));

}

const char * ltr_getitemN(json_object * handle,int row, const char * colname)
{
    int col;

    col = ltr_getcolidx(handle, colname);
    return ltr_getitem(handle,row,col);
}

static json_object * ltr_newMainRowByData(json_object * dataArray, int row)
{
    json_object * newRowObj;
    json_object * newItemArray;
    json_object * newItem;
    json_object * dataItemArray;
    json_object * dataItem;
    int c, ccount;

    newRowObj = jo_new_object();
    joo_set_int(newRowObj, "s", 0);
    newItemArray = jo_new_array();

    dataItemArray = joa_get_idx(dataArray, row);
    ccount = joa_length(dataItemArray);

    for(c = 0; c < ccount; c++)
    {
        newItem = jo_new_object();
        joo_set_int(newItem, "s", 0);

        dataItem = joa_get_idx(dataItemArray, c);
        joo_set_string(newItem, "v", jo_get_string(dataItem));

        joa_add(newItemArray,newItem);
    }

    joo_add(newRowObj, "r", newItemArray);

    return newRowObj;
}

static int ltr_convertDataToMain(json_object * handle)
{
    json_object * mainArray;
    json_object * dataArray;
    json_object * rowObj;
    int r,rcount;

    mainArray = jo_new_array();

    dataArray = joo_get(handle, "data");
    rcount = joa_length(dataArray);

    for(r = 0; r < rcount; r++)
    {
        rowObj = ltr_newMainRowByData(dataArray, r);
        joa_add(mainArray, rowObj);
    }

    joo_add(handle, "main", mainArray);

    return 0;
}

/*****************************************************
 * edit mode 
 ****************************************************/
int ltr_setmode(json_object * handle, int edit)
{
    joo_set_int(handle, "mode", edit);
    if(edit)
    {
        ltr_convertDataToMain(handle);

        joo_add(handle, "del", jo_new_array());
    }
    return 0;
}

static int ltr_insertrowEdit(json_object * handle, int row)
{
    int i;
    json_object * columnArray;
    json_object * mainArray;
    json_object * rowObj;
    json_object * itemArray;
    json_object * itemObj;

    int columncount;
    int rowcount;

    if(!handle )
        return LT_ERR_PARAM;
    
    columnArray = joo_get(handle, "column");
    mainArray = joo_get(handle, "main");

    columncount = joa_length(columnArray);
    rowcount = joa_length(mainArray);
    if(row < 0 || row >= rowcount)
    {
        row= rowcount;
    }

    rowObj = jo_new_object();
    joo_set_int(rowObj, "s", -1);
    itemArray = jo_new_array();

    for(i = 0; i < columncount; i++)
    {
        itemObj = jo_new_object();
        joo_set_int(itemObj, "s", 0);
        joo_set_string(itemObj, "v", "");
        joa_add(itemArray, itemObj);
    }

    joo_add(rowObj, "r", itemArray);

    joa_insert_idx(mainArray, row, rowObj);

    return row;
}

static int ltr_delrowEdit(json_object * handle, int row)
{
    json_object * mainArray;
    json_object * delArray;
    json_object * rowObj;
    int rowcount;
    int s;

    mainArray = joo_get(handle, "main");

    rowcount = joa_length(mainArray);
    if(row < 0 || row >= rowcount)
    {
        return LT_ERR_PARAM;
    }
   
    rowObj = joa_remove(mainArray, row);

    s = joo_get_int(rowObj, "s");
    if(s == -1 || s == -2)
    {
        jo_put(rowObj);
        return 0;
    }

    delArray = joo_get(handle, "del");
    joa_add(delArray, rowObj);
    
    return 0;
}

static int ltr_getrowcountEdit(json_object * handle)
{
    struct json_object * mainArray;

    mainArray = joo_get(handle, "main");

    return joa_length(mainArray);
}

static const char * ltr_getitemEdit(json_object * handle,int row, int col)
{
    struct json_object * columnObj;
    struct json_object * mainArray;
    struct json_object * rowObj;
    struct json_object * itemArray;
    struct json_object * itemObj;
    int columncount;
    int rowcount;
    
    columnObj = joo_get(handle, "column");
    mainArray = joo_get(handle, "main");

    columncount = joa_length(columnObj);
    rowcount = joa_length(mainArray);
    if(row < 0 || row >= rowcount)
    {
        return NULL;
    }

    if(col < 0 || col >= columncount)
    {
        return NULL;
    }

    rowObj = joa_get_idx(mainArray, row);

    itemArray = joo_get(rowObj, "r");
    itemObj = joa_get_idx(itemArray, col);
    return joo_get_string(itemObj, "v");
}

static int ltr_setitemEdit(json_object * handle,int row, int col, const char * val)
{
    struct json_object * columnArray;
    struct json_object * columnObj;
    struct json_object * mainArray;
    struct json_object * rowObj;
    struct json_object * itemArray;
    struct json_object * itemObj;
    int columncount;
    int rowcount;
    int s;
    const char * v;
    
    columnArray = joo_get(handle, "column");
    mainArray = joo_get(handle, "main");

    columncount = joa_length(columnArray);
    rowcount = joa_length(mainArray);
    if(row < 0 || row >= rowcount)
    {
        return LT_ERR_PARAM;
    }

    if(col < 0 || col >= columncount)
    {
        return LT_ERR_PARAM;
    }

    columnObj = joa_get_idx(columnArray, col);
    if(joo_get_int(columnObj, "u") == 0)
        return LT_ERR_DENIED;

    rowObj = joa_get_idx(mainArray, row);

    itemArray = joo_get(rowObj, "r");
    itemObj = joa_get_idx(itemArray, col);
    s = joo_get_int(itemObj, "s");
    v = joo_get_string(itemObj, "v");

    if(strcmp(v, val) == 0)
        return 0;
    if(s == 0)
    {
        joo_set_string(itemObj, "ov", v);
        joo_set_int(itemObj, "s", 1);
    }
    joo_set_string(itemObj, "v", val);

    s = joo_get_int(rowObj, "s");
    if(s == 0)
    {
        joo_set_int(rowObj, "s", 1);
    }
    else if(s == -1)
    {
        joo_set_int(rowObj, "s", -2);
    }

    return 0;
}

int ltr_setkey(json_object * handle, const char * key)
{
    joo_set_string(handle, "key", key);

    return 0;
}

int ltr_settable(json_object * handle, const char * table)
{
    joo_set_string(handle, "table", table);
    return 0;
}

int ltr_save(json_object * handle)
{
    json_object * mainArray;
    json_object * rowObj;
    json_object * itemArray;
    json_object * itemObj;
    int r,c;
    int rcount,ccount;
    int s;
    //main
    mainArray = joo_get(handle, "main");
    rcount = joa_length(mainArray);
    ccount = joa_length(joo_get(handle, "column"));

    for(r = rcount - 1; r >= 0; r--)
    {
        rowObj = joa_get_idx(mainArray, r);
        s = joo_get_int(rowObj, "s");
        if(s == -1)
        {
            joa_del_idx(mainArray, r);
            continue;
        }

        joo_set_int(rowObj, "s", 0);
        itemArray = joo_get(rowObj, "r");
        for(c = 0; c < ccount; c++)
        {
            itemObj = joa_get_idx(itemArray, c);
            joo_set_int(itemObj, "s", 0);
        }
    }

    //del
    joo_add(handle, "del", jo_new_array());

    return 0;
}