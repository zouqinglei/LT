#include "stdio.h"
#include "stdlib.h"
#include "myltr_sql.h"

/*
recordset:
{
    "mode": 1,
    "column":[{"n":"a", "u": 1},{"n":"b", "u": 1},{"n":"c", "u": 1},{"n":"d", "u": 1}],
    "key": "xxx",
    "table": "updatetable",
    "data":[
         [],
         []
        ]

    "main":[
        {"s":0, "r": [{"s":0, "v": "xxx", "ov": "oldvalue"},{item},...]},
        ...
        ]

    "del":[
        {"s":0, "r": [{"s":0, "v": "xxx", "ov": "oldvalue"},{item},...]},
        ...
    ]
}
*/

static const char * getColumnName(json_object * columnArray, int idx)
{
    return joo_get_string(joa_get_idx(columnArray, idx), "n");
}


static int getColumnUpdateable(json_object * columnArray, int idx)
{
    return joo_get_int(joa_get_idx(columnArray, idx), "u");
}

static int getColumnInsertable(json_object * columnArray, int idx)
{
    return joo_get_int(joa_get_idx(columnArray, idx), "i");
}

static int ltr_genSqlMain(json_object * handle, json_object * rowObj, int ccount)
{
    json_object * columnArray;
    json_object * itemArray;
    json_object * itemObj;
    int s;
    char * sql;
    char * set;
    char * setsql;
    const char * field;
    const char * v;
    const char * key;
    const char * keyval;
    json_object * keyObj;
    int keyindex;
    int i;
    int setlen;
    int totallen;
    int pos;
    int len;

    columnArray = joo_get(handle, "column");
    itemArray = joo_get(rowObj, "r");

    totallen = 0;
    s = joo_get_int(rowObj, "s");
    if(s == 1) //update
    {
        for(i = 0; i < ccount; i++)
        {
            if(getColumnUpdateable(columnArray, i) == 0)
                continue;
            itemObj = joa_get_idx(itemArray, i);
            s = joo_get_int(itemObj, "s");
            if(s == 1)
            {
                field = getColumnName(columnArray, i);
                v = joo_get_string(itemObj, "v");
                setlen = strlen(field) + strlen(v) + 7;
                set = (char *)malloc(setlen + 1);
                sprintf(set, " %s = '%s',", field, v);
                joo_set_string(itemObj, "set", set);
                joo_set_int(itemObj, "setlen", setlen);
                free(set);
                totallen += setlen;
            }
        }

        totallen += 256;
        setsql = (char *)malloc(totallen);
        pos = 0;
        for(i = 0; i < ccount; i++)
        {
            if(getColumnUpdateable(columnArray, i) == 0)
                continue;
            itemObj = joa_get_idx(itemArray, i);
            s = joo_get_int(itemObj, "s");
            if(s == 1)
            {
                set = (char *)joo_get_string(itemObj, "set");
                setlen = joo_get_int(itemObj, "setlen");
                BCOPY(set, setsql + pos, setlen);
                pos += setlen;
            }
        }
        setsql[pos - 1] = '\0';

        key = joo_get_string(handle, "key");
        keyindex = ltr_getcolidx(handle, key);
        keyObj = joa_get_idx(itemArray, keyindex);
        if(joo_get_int(keyObj, "s") == 0)
        {
            keyval = joo_get_string(keyObj, "v");
        }
        else
        {
            keyval = joo_get_string(keyObj, "ov");
        }

        sql = (char *) malloc(totallen + 256);
        sprintf(sql, " UPDATE %s SET %s WHERE %s = '%s';\n", 
            joo_get_string(handle, "table"),
            setsql, 
            key,
            keyval);

        free(setsql);
        len = strlen(sql);
        joo_set_string(rowObj, "sql", sql);
        joo_set_int(rowObj, "sqllen", len);
        free(sql);
    }
    else if(s == -2) //insert
    {
        totallen = 0;
        for(i = 0; i < ccount; i++)
        {
            if(getColumnInsertable(columnArray, i) == 0)
                continue;
            itemObj = joa_get_idx(itemArray, i);
            
            field = getColumnName(columnArray, i);
            v = joo_get_string(itemObj, "v");
            
            totallen += strlen(field) + strlen(v);         
        }

        sql = (char *)malloc(totallen + 256);
        sprintf(sql, "INSERT INTO %s (", joo_get_string(handle,"table"));
        pos = strlen(sql);
        
        for(i = 0; i < ccount; i++)
        {
            if(getColumnInsertable(columnArray, i) == 0)
                continue;
            field = getColumnName(columnArray, i);
            len = strlen(field);
            BCOPY(field, sql + pos, len);
            pos += len;
            BCOPY(",", sql+pos, 1);
            pos++;
        }

        pos--;

        BCOPY(" ) VALUES(\"", sql+pos, 11);
        pos += 11;
        for(i = 0; i < ccount; i++)
        {
            if(getColumnInsertable(columnArray, i) == 0)
                continue;

            itemObj = joa_get_idx(itemArray, i);
            v = joo_get_string(itemObj, "v");
            len = strlen(v);
            BCOPY(v, sql + pos, len);
            pos += len;
            BCOPY("\",\"", sql+pos, 3);
            pos+=3;
        }

        pos -= 2;
        BCOPY(");\n", sql+pos, 3);
        pos += 3;
        sql[pos] = '\0';
        len = strlen(sql);
        joo_set_string(rowObj, "sql", sql);
        joo_set_int(rowObj, "sqllen", len);
        free(sql);
    }
    else
    {
        len = 0;
        joo_set_int(rowObj, "sqllen", 0);
        joo_set_string(rowObj, "sql", "");
    }

    
    return len;
}

static int ltr_genSqlDel(json_object * handle, json_object * rowObj, int ccount)
{
    json_object * columnArray;
    json_object * itemArray;

    int s;
    const char * key;
    const char * keyval;
    json_object * keyObj;
    int keyindex;

    char delsql[1024];
    int len;

    columnArray = joo_get(handle, "column");
    itemArray = joo_get(rowObj, "r");


    s = joo_get_int(rowObj, "s");
    if(s != -1) //update
    {
        key = joo_get_string(handle, "key");
        keyindex = ltr_getcolidx(handle, key);
        keyObj = joa_get_idx(itemArray, keyindex);
        if(joo_get_int(keyObj, "s") == 0)
        {
            keyval = joo_get_string(keyObj, "v");
        }
        else
        {
            keyval = joo_get_string(keyObj, "ov");
        }

        sprintf(delsql, "DELETE FROM %s WHERE %s = '%s'",
            joo_get_string(handle, "table"),
            key,
            keyval);

        len = strlen(delsql);

        joo_set_string(rowObj, "sql", delsql);
        joo_set_int(rowObj, "sqllen", len);
    }

    return len;
}

static int ltr_combinSql(json_object * handle, char * sql)
{
    json_object * mainArray;
    json_object * delArray;
    json_object * rowObj;

    int r;
    int rcount;
    const char * rowSql;
    int rowSqlLen;
    int pos;

    pos = 0;
    //main
    mainArray = joo_get(handle, "main");
    rcount = joa_length(mainArray);
  
    for(r =0; r < rcount; r++)
    {
        rowObj = joa_get_idx(mainArray, r);
        rowSqlLen = joo_get_int(rowObj, "sqllen");
        rowSql = joo_get_string(rowObj, "sql");
        BCOPY(rowSql, sql + pos, rowSqlLen);
        pos += rowSqlLen;
    }

    //del
    delArray = joo_get(handle, "del");
    rcount = joa_length(delArray);
    for(r =0; r < rcount; r++)
    {
        rowObj = joa_get_idx(delArray, r);
        rowSqlLen = joo_get_int(rowObj, "sqllen");
        rowSql = joo_get_string(rowObj, "sql");
        BCOPY(rowSql, sql + pos, rowSqlLen);
        pos += rowSqlLen;
    }

    sql[pos] = '\0';

    return 0;
}

const char * sqlite3_getsql(json_object * handle)
{
    json_object * mainArray;
    json_object * delArray;
    json_object * rowObj;

    int r;
    int rcount,ccount;
    char * sql;
    int sqllen;

    //main
    mainArray = joo_get(handle, "main");
    rcount = joa_length(mainArray);
    ccount = joa_length(joo_get(handle, "column"));

    sqllen = 0;
    for(r =0; r < rcount; r++)
    {
        rowObj = joa_get_idx(mainArray, r);
        sqllen += ltr_genSqlMain(handle, rowObj, ccount);
        
    }

    //del
    delArray = joo_get(handle, "del");
    rcount = joa_length(delArray);
    for(r =0; r < rcount; r++)
    {
        rowObj = joa_get_idx(delArray, r);
        sqllen += ltr_genSqlDel(handle, rowObj, ccount);
        
    }

    sql = (char *)malloc(sqllen + 1);

    ltr_combinSql(handle, sql);

    joo_set_string(handle, "sql", sql);

    return sql;
}
