/*
 * lt_comcircbuf.h
 * circular buffer func
 * write by zouql 20080306
 */

#ifndef LT_COMMCIRCBUF_H_
#define LT_COMMCIRCBUF_H_

typedef struct
{
    int size;     /* buf max size */
    int start;    /* start of data */
    int len;      /* data length */
    char *data;   /* data buffer */
}CIRCBUF;

#define TCPIP_TELNET_IAC ((unsigned char)('\xff'))
#define TCPIP_TELNET_EOR ((unsigned char)('\xef'))

#ifdef __cplusplus
extern "C" {
#endif

int lt_circbufAlloc(CIRCBUF **cb,int size);
int lt_circbufFree(CIRCBUF *cb);
int lt_circbufReset(CIRCBUF *cb);
int lt_circbufPutch(CIRCBUF *cb, char ch);
int lt_circbufGetch(CIRCBUF *cb,char *ch);
int lt_circbufPeekch(CIRCBUF *cb,int pos,char *ch);
int lt_circbufUpdch(CIRCBUF *cb,int pos,char ch);
int lt_circbufPut(CIRCBUF *cb,int len,char *data);
int lt_circbufGet(CIRCBUF *cb,int len,char *data);
int lt_circbufPeek(CIRCBUF *cb,int pos,int len,char *data);
int lt_circbufUpd(CIRCBUF *cb,int pos,int len,char *data);
int lt_circbufDelete(CIRCBUF *cb,int len);
int lt_circbufBytesleft(CIRCBUF *cb);
int lt_circbufCrtlen(CIRCBUF *cb);
int lt_circbufIacEorlen(CIRCBUF *cb);
void lt_circbufIacEorDecode( char *bufsrc,int lensrc,char *bufdst,int *lendst);

int lt_circbufFindLen(CIRCBUF* cb,char ch);
int lt_circbufEOLLen(CIRCBUF* cb);

int lt_circbufFind(CIRCBUF* cb, char * str);

int lt_Val1(char c);
char lt_Ch1( int val );
char * memstr(const char * str1, int nLen1, const char * str2);
#ifdef __cplusplus
}
#endif
#endif /*LT_COMMCIRCBUF_H_*/
