/* 
 * lt_mq_server.h
 * write by zouql 20140724
 * zouqinglei@163.com
 * serve socket module
 */

#include "lt_c_foundation.h"


#ifndef LT_MQ_SERVER_H
#define LT_MQ_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

int ltmq_server_init();
int ltmq_server_destroy();
LT_Server * ltmq_server_instance();


#ifdef __cplusplus
}
#endif

#endif /*LT_MQ_SERVER_H*/
