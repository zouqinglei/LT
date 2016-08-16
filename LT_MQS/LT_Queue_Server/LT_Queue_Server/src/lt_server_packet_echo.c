
#include "lt_server_packet_echo.h"
#include "lt_commsocket.h"

int server_packet_echo_parse(CIRCBUF * pbuf, void ** data, int * datalen)
{
    int len;
  

    len = lt_circbufEOLLen(pbuf);
  
    if(len > 0)
    {
        *data = malloc(len + 1);
        *datalen = len + 1;

        lt_circbufGet(pbuf,len,*data);
        ((char *)(*data))[len] = '\0';

		printf("recv: %s\n",(char *)(*data));
        return 0;
    }

    return LT_ERR_BUF;
}


int server_packet_echo_packet(void * data, int datalen, char ** buf, int * buflen)
{
    if(data == NULL)
        return -1;

    *buf = (char *)malloc(datalen);
    if(*buf == NULL)
        return -1;

    *buflen = datalen;

    BCOPY(data, *buf, datalen);

    return 0;
}

void server_packet_echo_free(void * data)
{
    free(data);
}


int server_packet_echo_active(char ** buf, int * buflen)
{

    return -1;
}


void server_packet_echo_process(LT_ClientInfo * pInfo)
{
	int i;
	pInfo->replydata = malloc(pInfo->requestlen);
	for(i = 0; i < pInfo->requestlen; i++)
	{
	
		((char *)pInfo->replydata)[i] = (char)toupper(((char*)pInfo->requestdata)[i]);
	}

	pInfo->replylen = pInfo->requestlen;
    if(pInfo->clientID % 2 == 0)
    {
        pInfo->closeclient = 1;
    }

}

void server_packet_echo_connect_func(LT_ClientInfo * pInfo)
{
    printf("client [%d] from %s connect.\n",pInfo->clientID, inet_ntoa(pInfo->clientaddr.sin_addr)); 
}

void server_packet_echo_close_func(LT_ClientInfo * pInfo)
{
    printf("client [%d] from %s close.\n",pInfo->clientID, inet_ntoa(pInfo->clientaddr.sin_addr)); 
}