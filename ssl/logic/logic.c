#include "../logic.h"
#include "log.h"
#include "ThreadPool.h"
#include <time.h>
#include <sys/time.h>
#include <stddef.h>


typedef struct {
	void *_ctx;
	void *_ret;
	UINT64 _begin_time;
	UINT64 _end_time;
}SSL_NEW;

typedef struct {
	void *_ssl;
	UINT64 _begin_time;
}SSL_FREE;

typedef struct {
	void *_ssl;
	int _ret;
	UINT64 _begin_time;
	UINT64 _end_time;
}SSL_CONNECT;

typedef struct {
	void * _ssl;
	int _ret;
	int _len;
	UINT64 _begin_time;
	UINT64 _end_time;
	void *_buf;
}SSL_READ;



typedef struct {
	void * _ssl;
	int _ret;
	int _len;
	UINT64 _begin_time;
	UINT64 _end_time;
	void *_buf;
}SSL_WRITE;

UINT64 nb_getSysTime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	UINT64 tm = (tv.tv_sec) * 1000000 + tv.tv_usec;
//	char buf[64];
//	snprintf(buf, sizeof(buf), "nb_getSysTime---%u,%u", tv.tv_sec,tv.tv_usec);
//	flog(buf);
	return tm;
}

void * myprocess (void *arg, int mode)
{
	switch (mode){
		case 0:{
			SSL_NEW *obj = (SSL_NEW*)arg;
			flog("ssl_new");
		}
			break;
		case 1:{
			flog("ssl_free");
		}
			break;
		case 2:{
			flog("ssl_connect");
		}
			break;
		case 3:{
			flog("ssl_read");
		}
			break;
		case 4:{
			flog("ssl_write");
		}
			break;
	}
    return NULL;
}

void nb_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time){
	SSL_NEW *obj = (SSL_NEW*)malloc(sizeof(SSL_NEW));
	obj->_begin_time = start_time;
	obj->_ctx= ctx;
	obj->_end_time = end_time;
	obj->_ret = ret;
	pool_add_worker(myprocess,obj,0);
}

void nb_ssl_close(void *ssl, UINT64  start_time){
	SSL_FREE *obj = (SSL_FREE*)malloc(sizeof(SSL_FREE));
	obj->_begin_time = start_time;
	obj->_ssl = ssl;
	pool_add_worker(myprocess,obj,1);
}
void nb_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time){
	SSL_CONNECT *obj = (SSL_CONNECT*)malloc(sizeof(SSL_CONNECT));
	obj->_begin_time = start_time;
	obj->_ssl= ssl;
	obj->_end_time = end_time;
	obj->_ret = ret;
	pool_add_worker(myprocess,obj,2);
}

void nb_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
	SSL_READ *obj = (SSL_READ*)malloc(sizeof(SSL_READ));
	obj->_begin_time = start_time;
	obj->_ssl= ssl;
	obj->_end_time = end_time;
	obj->_ret = ret;
	obj->_buf = (void*)malloc(num);
	memcpy(obj->_buf, buf, num);
	obj->_len = num;
	pool_add_worker(myprocess,obj,3);
}
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
	SSL_WRITE *obj = (SSL_WRITE*)malloc(sizeof(SSL_WRITE));
	obj->_begin_time = start_time;
	obj->_ssl= ssl;
	obj->_end_time = end_time;
	obj->_ret = ret;
	obj->_buf = (void*)malloc(num);
	memcpy(obj->_buf, buf, num);
	obj->_len = num;
	pool_add_worker(myprocess,obj,4);
}

