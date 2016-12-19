#ifndef NB_LOGIC_H
#define NB_LOGIC_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "./logic/entity.h"



long long nb_getSysTime();
void nb_ssl_create(void *ctx, void *ret, int64 start_time, int64 end_time);
void nb_ssl_close(void *ssl, int64 start_time);
void nb_ssl_connect(void *ssl, int ret, int64 start_time, int64 end_time);
void nb_ssl_read(void *ssl,void *buf,int num, int ret, int64 start_time, int64 end_time);
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, int64 start_time, int64 end_time);
void nb_ssl_notify(void *ssl, HandleMessageFN func);

#ifdef  __cplusplus
}
#endif

#endif
