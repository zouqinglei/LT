/************************************************************************/
/* printtable.c 
   write by zouqinglei@163.com 20141024
   All right reserved.
*/
/************************************************************************/
#include "printtable.h"

static int calcColWidth(json_object * hRecord,int col)
{
    int row, rowCount;
    int width, widthItem;
    const char * colname;
    const char * item;

    colname = ltr_getcolname(hRecord, col);
    width = strlen(colname);

    rowCount = ltr_getrowcount(hRecord);
    for(row = 0; row < rowCount; row++)
    {
        item = ltr_getitem(hRecord, row, col);
        widthItem = strlen(item);
        if(widthItem > width)
        {
            width = widthItem;
        }
    }

    return width; 
}

static int calc_cols_width(json_object * hRecord, struct json_object * widthsObj)
{
    int col, colCount;
    int width;

    colCount = ltr_getcolumncount(hRecord);
    for(col = 0; col < colCount; col++)
    {
        width = calcColWidth(hRecord, col);
        joa_add(widthsObj, jo_new_int(width));
    }
    return 0;
}

static int print_dashes(json_object * hRecord, struct json_object * widthsObj)
{
	int col, colCount;
    int width;
    int i;

    colCount = ltr_getcolumncount(hRecord);

	fputc('+',stdout);
	for(col = 0; col < colCount; col++)
	{
        width = jo_get_int(joa_get_idx(widthsObj, col));
	    for(i = 0; i < width + 2; i++)
		    fputc('-',stdout);
	    fputc('+',stdout);	
	}
    fputc('\n',stdout);	

	return 0;
}

int  print_table(json_object * hRecord)
{
    int row, rowCount;
    int col, colCount;
    int width;
	int i;
    struct json_object * widthsObj;
	if(!hRecord)
		return -1;
	
	/*calc col width max size */
	widthsObj = jo_new_array();
    calc_cols_width(hRecord, widthsObj);

    /* print head*/
    print_dashes(hRecord, widthsObj);
	fputc('|',stdout);
    
    colCount = ltr_getcolumncount(hRecord);

	for(col = 0; col < colCount; col++)
	{
        width = jo_get_int(joa_get_idx(widthsObj, col));
		printf(" %-*s |", width, ltr_getcolname(hRecord, col));
	}

	fputc('\n',stdout);

    /* print body */
    rowCount = ltr_getrowcount(hRecord);
	print_dashes(hRecord, widthsObj);
    
	for(row = 0; row < rowCount; row++)
	{
		i = 0;
		fputc('|',stdout);
        for(col = 0; col < colCount; col++)
		{
			width = jo_get_int(joa_get_idx(widthsObj, col));
			printf(" %-*s |", width, ltr_getitem(hRecord,row,col));
		}
		
		fputc('\n',stdout);
	}
	if(rowCount > 0)
	    print_dashes(hRecord, widthsObj);
	printf("total %d.\n",rowCount);
	return 0;
}


