#ifndef NB_LOGIC_H
#define NB_LOGIC_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif


UINT64 nb_getSysTime(); 
void nb_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time);
void nb_ssl_close(void *ssl, UINT64 start_time);
void nb_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd);
void nb_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time);
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time);
void nb_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time);


#ifdef  __cplusplus
}
#endif

#endif
