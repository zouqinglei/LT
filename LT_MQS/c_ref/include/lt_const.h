/*
    lt_const.h
    some const definition 

    write by zouql  20131117
    zouqinglei@163.com 
    All right reserved.
*/

#ifndef LT_CONST_H__
#define LT_CONST_H__

/*ERROR CODE*/
#define LT_ERR_NORMAL     0   /*OK*/
#define LT_ERR_SUCCESS    0
#define LT_ERR_FAIL      -1
#define LT_ERR_LARGEDATA -2   /*DATA Too MORE */
#define LT_ERR_DENIED    -3   /*Permission denied*/
#define LT_ERR_SYSTEM    -4   /*system error*/
#define LT_ERR_NOENT     -5   /*server found no entry*/
#define LT_ERR_CONNECT   -6   /*connect error*/
#define LT_ERR_TOMUCH    -7
#define LT_ERR_MALLOC    -8   /*memory alloc error*/
#define LT_ERR_SNDDAT	 -9   /*send data error or net failure*/
#define LT_ERR_RCVDAT    -10
#define LT_ERR_REGSVC    -11  /*register service error*/
#define LT_ERR_BUF       -12
#define LT_ERR_TASK      -13
#define LT_ERR_PARAM     -14   /*bad parameter */
#define LT_ERR_TIMEOUT   -15   /* timeout */
#define LT_ERR_EXCEPTION -16
#define LT_ERR_EXIST     -17   /* alread exist */
#define LT_ERR_NODATA    -18
#define LT_ERR_PACKET    -19
#define LT_ERR_RETRY     -20

#endif /*LT_CONST_H__*/
