/*
    myClient.h
    write by zouql 20151029
    zouqinglei@163.com
*/

#ifndef INTERCLIENT_MY_CLIENT_H
#define INTERCLIENT_MY_CLIENT_H



#ifdef __cplusplus
extern "C" {
#endif


void clientInit(const char * hostIP, unsigned short port);
void clientConnect();
void clientDisConnect();
void clientDestroy();

void clientSend(const char * name, const char * message);



#ifdef __cplusplus
}
#endif

#endif /*INTERCLIENT_MY_CLIENT_H*/