#include "../logic.h"
#include "ThreadPool.h"

#include <stdio.h>
#include "Handler.h"
#include "gettime.h"
#include "entity.h"
#include <stdlib.h>
#include <malloc.h>


long long nb_getSysTime(){
	return getSysTime();
}

void * myprocess (void *arg, int mode)
{
	switch (mode){
		case 0:{
			handle_ssl_new((SSL_NEW*)arg);
		}
			break;
		case 1:{
			handle_ssl_free((SSL_FREE*)arg);
		}
			break;
		case 2:{
			handle_ssl_connect((SSL_CONNECT*)arg);
		}
			break;
		case 3:{
			handle_ssl_read((SSL_READ*)arg);
		}
			break;
		case 4:{
			handle_ssl_write((SSL_WRITE*)arg);
		}
			break;
		case 5:{
			handle_ssl_notify((SSL_NOTIFY*)arg);
		}
			break;
	}
    return NULL;
}

void nb_ssl_create(void *ctx, void *ret, int64 start_time, int64 end_time){
	SSL_NEW *obj = (SSL_NEW*)malloc(sizeof(SSL_NEW));
	obj->_begin_time = start_time;
	obj->_ctx= ctx;
	obj->_end_time = end_time;
	obj->_ret = ret;
	pool_add_worker(myprocess,obj,0);
}

void nb_ssl_close(void *ssl, int64  start_time){
	SSL_FREE *obj = (SSL_FREE*)malloc(sizeof(SSL_FREE));
	obj->_begin_time = start_time;
	obj->_ssl = ssl;
	pool_add_worker(myprocess,obj,1);
}
void nb_ssl_connect(void *ssl, int ret, int64 start_time, int64 end_time){
	SSL_CONNECT *obj = (SSL_CONNECT*)malloc(sizeof(SSL_CONNECT));
	obj->_begin_time = start_time;
	obj->_ssl= ssl;
	obj->_end_time = end_time;
	obj->_ret = ret;
	pool_add_worker(myprocess,obj,2);
}

void nb_ssl_read(void *ssl,void *buf,int num, int ret, int64 start_time, int64 end_time){
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
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, int64 start_time, int64 end_time){
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

void nb_ssl_notify(void *ssl, HandleMessageFN func){
	SSL_NOTIFY *obj = (SSL_NOTIFY*)malloc(sizeof(SSL_NOTIFY));
	obj->_ssl= ssl;
	obj->_notify = func;
	pool_add_worker(myprocess,obj,5);
}
