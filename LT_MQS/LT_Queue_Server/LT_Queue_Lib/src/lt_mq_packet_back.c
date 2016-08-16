/* 
 * lt_mq_packet.c
 * write by zouql 20140724
 * zouqinglei@163.com
 * packet 
 */

#include "lt_mq_packet.h"


LTMQ_UserData * ltmq_userdata_new()
{
    LTMQ_UserData * pData;
    pData = (LTMQ_UserData *)malloc(sizeof(LTMQ_UserData));
    BZERO(pData, sizeof(LTMQ_UserData));

    pData->headobj = jo_new_object();
    
    joo_add(pData->headobj, "param", jo_new_object());

    return pData;
}

void ltmq_userdata_free(void * userdata)
{
    LTMQ_UserData * pData;
 
    pData = (LTMQ_UserData *)userdata;

    if(pData)
    {
        if(pData->headobj)
            jo_put(pData->headobj);

        if(pData->blob)
            free(pData->blob);

        free(pData);
    }
}

/*
 * server_packet_parse_func()
 * return 1: active packet
 *        0: if parse a packet to data
 *        LT_ERR_BUF: the buf size is not enought
 *        LT_ERR_PACKET: the packet is error.
 * if return 1: then the packet is a active packet.
 *
 */


int ltmq_packet_parse(CIRCBUF * pbuf, unsigned int * requestID, void ** userdata)
{
    int ret;
    int size;
    int tmp;
    int pos;

    LTMQ_PacketLead lead;
    LTMQ_UserData * pData;

    char * buffer;
    char * cheadbuf;
    char * cblobbuf;
    int cheadlen;
    int cbloblen;

    /* un compress */

    
    size = lt_circbufCrtlen(pbuf);
    if(size < LTMQ_PACKET_LEAD_SIZE)
        return LT_ERR_BUF;

    pos = 0;
    
    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.magic = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.protocol = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.reqid = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.flag = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.headlen = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.oriheadlen = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.bloblen = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.oribloblen = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.reserved0 = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.reserved1 = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.reserved2 = ntohl(tmp);
    pos += 4;

    lt_circbufPeek(pbuf,pos,4,(char *)&tmp);
    lead.reserved3 = ntohl(tmp);
    pos += 4;

 
    if((lead.magic !=  0x4C544D51) ||
        (lead.headlen > LTMQ_PACKET_HEAD_SIZE_MAX) ||
        (lead.bloblen > LTMQ_PACKET_BLOB_SIZE_MAX))
    {
        return LT_ERR_PACKET;
    }

    if(size < (LTMQ_PACKET_LEAD_SIZE + lead.headlen + lead.bloblen))
    {
        return LT_ERR_BUF;
    }

    lt_circbufDelete(pbuf,LTMQ_PACKET_LEAD_SIZE);

    if(lead.flag & LTMQ_PACKET_FLAG_ACTIVE)
    {
        return 1;
    }


    pData = (LTMQ_UserData *)malloc(sizeof(LTMQ_UserData));
    BZERO(pData, sizeof(LTMQ_UserData));


    buffer = (char *)malloc(lead.headlen);
    lt_circbufGet(pbuf, lead.headlen, buffer);

    //head if compress
    if(lead.flag & LTMQ_PACKET_FLAG_ENCRYPT_HEAD) 
    {
        cheadlen = lead.oriheadlen + 4096;
        cheadbuf = (char *)malloc(cheadlen);
        ret = uncompress(cheadbuf, &cheadlen, buffer, lead.headlen);

        if(ret != Z_OK)
            return LT_ERR_PACKET;

        free(buffer);
        buffer = cheadbuf;
        
    }

    pData->headobj = json_tokener_parse(buffer);
    free(buffer);
    if(is_error(pData->headobj))
    {
        free(pData);
        return LT_ERR_PACKET;
    }

    //blob
    //blob if compress
    if(lead.flag & LTMQ_PACKET_FLAG_ENCRYPT_BLOB) 
    {
        buffer = (char *)malloc(lead.bloblen);
        lt_circbufGet(pbuf, lead.bloblen, buffer);

        cbloblen = lead.oribloblen + 4096;
        cblobbuf = (char *)malloc(cbloblen);
        ret = uncompress(cblobbuf, &cbloblen, buffer, lead.bloblen);

        if(ret != Z_OK)
            return LT_ERR_PACKET;

        free(buffer);
        pData->bloblen = lead.oribloblen;
        pData->blob = cblobbuf;
    }
    else
    {
        pData->blob = malloc(lead.bloblen);
        lt_circbufGet(pbuf, lead.bloblen, pData->blob);
    }

    
    *requestID = lead.reqid;
    *userdata = (void *)pData;

    return 0;
}


int ltmq_packet_packet(const unsigned int requestID, const void * userdata,  char ** buf, int * buflen)
{
    int ret;
    int tmp;
    LTMQ_UserData * pData;
    LTMQ_PacketLead lead;
    const char * pszHead;
    int pos;

    char * cheadbuf;
    int cheadlen;
    char * cblobbuf;
    int cbloblen;

    if(userdata == NULL)
        return -1;
    cheadbuf = NULL;
    cblobbuf = NULL;
    BZERO(&lead, sizeof(LTMQ_PacketLead));

    pData = (LTMQ_UserData *)userdata;

    pszHead = json_object_to_json_string(pData->headobj);
    lead.oriheadlen = strlen(pszHead) + 1;
    lead.headlen = lead.oriheadlen;

    lead.oribloblen = pData->bloblen;
    lead.bloblen = pData->bloblen;

    //compress
    if(lead.oriheadlen >= 1024)
    {
        lead.flag |= LTMQ_PACKET_FLAG_ENCRYPT_HEAD;
        cheadlen = lead.oriheadlen + 4096;
        cheadbuf = (char *)malloc(cheadlen);
        ret = compress(cheadbuf, &cheadlen, (unsigned char  *)pszHead, lead.oriheadlen);
        lead.headlen = cheadlen;
    }

    if(lead.oribloblen >= 1024)
    {
        lead.flag |= LTMQ_PACKET_FLAG_ENCRYPT_BLOB;
        cbloblen = lead.oribloblen + 4096;
        cblobbuf = (char *)malloc(cbloblen);
        ret = compress(cblobbuf, &cbloblen, (unsigned char  *)pData->blob, lead.oribloblen);
        lead.bloblen = cbloblen;
    }
        
    *buflen = LTMQ_PACKET_LEAD_SIZE + lead.headlen + lead.bloblen ;

    *buf = (char *)malloc(*buflen);

    pos = 0;
    /* magic */
    tmp = htonl(0x4C544D51);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;
    /* protocol */
    tmp = htonl(0x00010000);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reqid */
    tmp = htonl(requestID);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* flag */
    tmp = htonl(lead.flag);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* headlen */
    tmp = htonl(lead.headlen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* oriheadlen */
    tmp = htonl(lead.oriheadlen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* bloblen */
    tmp = htonl(lead.bloblen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* oribloblen */
    tmp = htonl(lead.oribloblen);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved0 */
    tmp = htonl(lead.reserved0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved1 */
    tmp = htonl(lead.reserved1);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved2 */
    tmp = htonl(lead.reserved2);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved3 */
    tmp = htonl(lead.reserved3);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    if(lead.flag & LTMQ_PACKET_FLAG_ENCRYPT_HEAD)
    {
        BCOPY(cheadbuf, *buf + LTMQ_PACKET_LEAD_SIZE, lead.headlen);
        free(cheadbuf);
    }
    else
    {
        BCOPY(pszHead, *buf + LTMQ_PACKET_LEAD_SIZE, lead.headlen);
    }

    if(lead.flag & LTMQ_PACKET_FLAG_ENCRYPT_BLOB)
    {
        BCOPY(cblobbuf, *buf + LTMQ_PACKET_LEAD_SIZE + lead.headlen, lead.bloblen);
        free(cblobbuf);
    }
    else
    {
        BCOPY(pData->blob, *buf + LTMQ_PACKET_LEAD_SIZE + lead.headlen, lead.bloblen);
    }
    
    
    return 0;
}

int ltmq_packet_active(char ** buf, int * buflen)
{
    int tmp;
    int pos;


    *buf = (char *)malloc(LTMQ_PACKET_LEAD_SIZE);
    if(*buf == NULL)
        return -1;

    *buflen = LTMQ_PACKET_LEAD_SIZE;

    pos = 0;
    /* magic */
    tmp = htonl(0x4C544D51);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;
    /* protocol */
    tmp = htonl(0x00010000);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reqid */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* flag */
    tmp = htonl(LTMQ_PACKET_FLAG_ACTIVE);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* headlen */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* oriheadlen */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* bloblen */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* oribloblen */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved0 */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved1 */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved2 */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    /* reserved3 */
    tmp = htonl(0);
    BCOPY(&tmp, *buf + pos, 4);
    pos += 4;

    return 0;
}

