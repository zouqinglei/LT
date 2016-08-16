/* 
 * lt_server_packet_echo.h
 * write by zouql 2013111
 *
 * for echo server packet 
 * packet operation functions
 */

#include "lt_serversocket.h"


#ifndef LT_SERVER_PACKET_ECHO_H
#define LT_SERVER_PACKET_ECHO_H


/*
 * server_packet_parse()
 * return 0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */
int server_packet_echo_parse(CIRCBUF * pbuf, void ** data, int * datalen);

/*
 * server_packet_packet()
 *
 * return 0: if packet success
 * else is error
 */
int server_packet_echo_packet(void * data, int datalen, char ** buf, int * buflen);


/*
 * server_packet_free()
 * free packet data memory
 * 
 */
void server_packet_echo_free(void * data);

/*
 * server_packet_active
 * create a active packet for send to client
 * if return 0,then success
 * else the active is not support 
 */
int server_packet_echo_active(char ** buf, int * buflen);

/*
 * server_packet_echo_process()
 * process the pInfo->requestdata and reply to the replydata
 */

void server_packet_echo_process(LT_ClientInfo * pInfo);

void server_packet_echo_connect_func(LT_ClientInfo * pInfo);

void server_packet_echo_close_func(LT_ClientInfo * pInfo);

#endif /*LT_PACKET_H*/
