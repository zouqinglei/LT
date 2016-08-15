/*
 * lt_commcircbuf.c
 * circular buffer func
 * write by zouql 20080306
     zouqinglei@163.com 
    All right reserved.
 */

#include "lt_commcircbuf.h"
#include "lt_commsocket.h"

/*
 * Allocate circular buffer.
 *
 * cb = allocated circular buffer
 * size = circular buffer size
 *
 * return = 0 or error code
 */
int lt_circbufAlloc(CIRCBUF **cb,int size)
{
    CIRCBUF *cbw;

    cbw = (void *)malloc(sizeof(CIRCBUF));
    if( cbw == (void *)NULL )
        return -1;

    cbw->data = (void *)malloc(size);
    if( cbw->data == (void *)NULL )
        return -1;

    cbw->size = size;
    cbw->start = 0;
    cbw->len = 0;

    *cb = cbw;

    return 0;
    
}

/*
 * Free circular buffer.
 *
 * cb = circular buffer
 *
 * return = 0 or error code
 */
int lt_circbufFree(CIRCBUF *cb)
{
    free( cb->data );
    free( cb );

    return 0;
    
}

/*
 * Reset circular buffer.
 *
 * cb = circular buffer
 *
 * return = 0 or error code
 *
 * All data contained in the buffer is deleted.
 */
int lt_circbufReset(CIRCBUF *cb)
{
    BZERO( cb->data,cb->size );
    cb->start = 0;
    cb->len = 0;

    return 0;
    
}

/*
 * Put character in circular buffer.
 *
 * cb = circular buffer
 * ch = character to be written
 *
 * return = 0 or error code
 */
int lt_circbufPutch(CIRCBUF *cb, char ch)
{
    int i;

    /* overflow test */
    if( cb->len == cb->size )
        return -1;

    /* compute position */
    i = cb->start + cb->len;
    if( i >= cb->size )
        i -= cb->size;

    /* put the character */
    cb->data[i] = ch;
    cb->len++;

    return 0;
}

/*
 * Get character from circular buffer.
 *
 * cb = circular buffer
 * ch = character read
 *
 * return = 0 or error code
 */
int lt_circbufGetch(CIRCBUF *cb,char *ch)
{
    /* test if buffer empty */
    if( cb->len == 0 )
        return -1;

    /* get character */
    *ch = cb->data[cb->start];
    cb->data[cb->start] = 0;

    /* update structure */
    cb->start++;
    if( cb->start == cb->size )
        cb->start = 0;
    cb->len--;

    return 0;    
}

/*
 * Peek character from circular buffer.
 *
 * cb = circular buffer
 * pos = relative position in buffer
 * ch = character peeked
 *
 * return = 0 or error code
 */
int lt_circbufPeekch(CIRCBUF *cb,int pos,char *ch)
{
    int i;

    /* test if position is in range */
    if( pos < 0 || pos > cb->len-1 )
        return -1;

    /* compute absolute position */
    i = cb->start + pos;
    if( i >= cb->size )
        i -= cb->size;

    /* peek the character */
    *ch = cb->data[i];

    return 0;
    
}

/*
 * Update character in circular buffer.
 *
 * cb = circular buffer
 * pos = relative position in buffer
 * ch = new value
 *
 * return = 0 or error code
 */
int lt_circbufUpdch(CIRCBUF *cb,int pos,char ch)
{
    int i;

    /* test if position is in range */
    if( pos < 0 || pos > cb->len-1 )
        return -1;

    /* compute absolute position */
    i = cb->start + pos;
    if( i >= cb->size )
        i -= cb->size;

    /* update character */
    cb->data[i] = ch;

    return 0;
}

/*
 * Put data in circular buffer.
 *
 * cb = circular buffer
 * len = data length
 * data = data buffer
 *
 * return = 0 or error code
 */
int lt_circbufPut(CIRCBUF *cb,int len,char *data)
{
    int i;

    /* check length */
    if( len < 0 )
        return -1;

    /* overflow test */
    if( cb->len + len > cb->size )
        return -1;

    /* put data */
    for( i=0; i<len; i++ )
        lt_circbufPutch( cb, data[i] );

    return 0;
}

/*
 * Get data from circular buffer.
 *
 * cb = circular buffer
 * len = data length
 * data = data buffer
 *
 * return = 0 or error code
 */
int lt_circbufGet(CIRCBUF *cb,int len,char *data)
{
    int i;

    /* check length */
    if( len < 0 || len > cb->len )
        return -1;

    /* get data */
    for( i=0; i<len; i++ )
        lt_circbufGetch( cb, &data[i] );

    return 0;
}

/*
 * Peek data from circular buffer.
 *
 * cb = circular buffer
 * pos = relative position in buffer
 * len = data length
 * data = data buffer
 *
 * return = 0 or error code
 */
int lt_circbufPeek(CIRCBUF *cb,int pos,int len,char *data)
{
    int i,rv;

    for( i=0; i<len; i++ )
    {
        rv = lt_circbufPeekch( cb, pos+i, &data[i] );
        if( rv == -1 )
            return -1;   
    }

    return 0;
}

/*
 * Update data in circular buffer.
 *
 * cb = circular buffer
 * pos = relative position in buffer
 * len = data length
 * data = data buffer
 *
 * return = 0 or error code
 */
int lt_circbufUpd(CIRCBUF *cb,int pos,int len,char *data)
{
    int i,rv;

    for( i=0; i<len; i++ )
    {
        rv = lt_circbufUpdch( cb, pos+i, data[i] );
        if( rv == -1 )
            return -1;   
    }

    return 0;
}

/*
 * Delete data from circular buffer.
 *
 * cb = circular buffer
 * len = number of bytes to be deleted
 *
 * return = 0 or error code
 */
int lt_circbufDelete(CIRCBUF *cb,int len)
{
    /* check length */
    if( len < 0 || len > cb->len )
        return -1;

    /* update structure */
    cb->start += len;
    if( cb->start >= cb->size )
        cb->start -= cb->size;
    cb->len -= len;

    return 0;
}

/*
 * Get free space size for a circular buffer.
 *
 * cb = circular buffer
 *
 * return = free space size
 */
int lt_circbufBytesleft(CIRCBUF *cb)
{
    return ( cb->size - cb->len );

}

/*
 * Get total data length for a circular buffer.
 *
 * cb = circular buffer
 *
 * return = total data length
 */
int lt_circbufCrtlen(CIRCBUF *cb)
{
    return ( cb->len );
}

/*
 * Get the lenth of the first IAC/EOR record from a circular buffer.
 *
 * cb = circular buffer
 *
 * return = record length
 */
int lt_circbufIacEorlen(CIRCBUF *cb)
{
    int len,iacflag,size,i;
    unsigned char c;

    iacflag=0;
    len = 0;
    size = cb->len;

    for( i=0; i<size; i++ )
    {
        lt_circbufPeekch( cb, i, (char *)&c );

        if( iacflag == 0 ) {
            if( c == TCPIP_TELNET_IAC )
                iacflag = 1;
        }
        else {
            if( c == TCPIP_TELNET_EOR ) {
                len = i+1;
                break;
            }
            iacflag = 0;
        }
    }
    return (len);
}

/*
 * Remove IAC commands from a data buffer; so far only
 * the sequences IAC,IAC and IAC,EOR need to be removed.
 *
 * bufsrc= input data buffer
 * lensrc= length of input data buffer
 * bufdst= output data buffer
 * lendst= output length of output data buffer
 */
void lt_circbufIacEorDecode( char *bufsrc,int lensrc,char *bufdst,int *lendst)
{
    int len,iacflag,i;
    unsigned char c;

    len = 0;
    iacflag = 0;

    for( i=0; i<lensrc; i++ )
    {
        c = bufsrc[i];
        if( iacflag==0 ) {
            if( c == TCPIP_TELNET_IAC )
                iacflag = 1;
            else
                bufdst[len++] = c;
        }
        else {
            if( c == TCPIP_TELNET_IAC )
                bufdst[len++] = c;
            iacflag = 0;
        }
    }
    *lendst = len;
}

int lt_circbufFindLen(CIRCBUF* cb,char ch)
{
    int len,size,i;
    unsigned char c;

   
    len = 0;
    size = cb->len;

    for( i=0; i<size; i++ )
    {
        lt_circbufPeekch( cb, i, (char *)&c );
        if( c == (unsigned char)ch)
        {
            len = i+1;
            break;
        }
    }
    return (len);
}

int lt_circbufEOLLen(CIRCBUF* cb)
{
    int len,size,i;
    unsigned char c;

   
    len = 0;
    size = cb->len;

    for( i=0; i<size; i++ )
    {
        lt_circbufPeekch( cb, i, (char *)&c );
        if( c == 0x0a || c == 0x0d)
        {
            len = i+1;
            break;
        }
    }
    return (len);
}

int lt_circbufFind(CIRCBUF* cb, char * str)
{
    int len,size;
    char * cp;

   
    len = 0;
    size = cb->len;

	cp = memstr(&cb->data[cb->start],cb->len,str);

	if(cp)
		return (cp - &cb->data[cb->start]); 

	return -1;	
}

/*
 * Get value from character (TESS format).
 */
int lt_Val1( char c )
{
    return ( (int)((unsigned char)c)-48 );
}

/*----------------------------------------------------------------------------*/
/*
 * Get character from value (TESS format).
 */
char lt_Ch1( int val )
{
    if( -48 <= val && val <= 211 )
        return( (char)( (unsigned char)(48+val) ) );
    else
        return (char)0;
}

/*
	在一段内存缓冲中查找指定字符串的位置，从头开始查找，不区分大小写。
	返回第一个找到的位置。
	str1 - 内存缓冲的头指针
	nLen1 - 内存缓冲长度
	str2 - 要查找匹配的字符串
*/
char * memstr(const char * str1, int nLen1, const char * str2)
{
	int ls1 = nLen1;
    char *cp = (char *) str1;
    char *s1, *s2;

    if ((NULL == str1) || (NULL == str2) || (nLen1 <= 0))
        return NULL;
 
    

    if ( !*str2 )
        return((char *)str1);

    while (ls1 > 0)
    {
        s1 = cp;
        s2 = (char *) str2;

        while ( *s1 && *s2 && !(*s1-*s2) )
            s1++, s2++;
 
        if (!*s2)
            return(cp);
 
        cp++;
        ls1--;
    }

    return(NULL);
}
