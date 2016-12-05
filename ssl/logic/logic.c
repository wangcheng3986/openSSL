#include "../logic.h"
#include "log.h"
#include "ThreadPool.h"
#include <time.h>
#include <sys/time.h>
#include <stddef.h>

UINT64 nb_getSysTime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	UINT64 tm = (tv.tv_sec) * 1000000 + tv.tv_usec;
	char buf[64];
	snprintf(buf, sizeof(buf), "nb_getSysTime---%u,%u", tv.tv_sec,tv.tv_usec);
	flog(buf);
	return tm;
}

void * myprocess (void *arg)
{
	flog ("threadid is working on task");
    return NULL;
}

void nb_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_create");
	pool_init();
	pool_add_worker(myprocess,0);
   // get_ssl_handler()->on_ssl_create(ctx, ret, start_time, end_time);
}

void nb_ssl_close(void *ssl, UINT64  start_time){
	flog("nb_ssl_close");
//    get_ssl_handler()->on_ssl_close(ssl, start_time);
}
void nb_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_connect");
//    LOGD("nb_ssl_connect");
   // get_ssl_handler()->on_ssl_connect(ssl,ret,start_time,end_time,fd);
}

void nb_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_read");
	pool_init();
	pool_add_worker(myprocess,0);
//    LOGD("nb_ssl_read");
   // get_ssl_handler()->on_ssl_read(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_write");
	pool_init();
	pool_add_worker(myprocess,0);
//    LOGD("nb_ssl_write");
   // get_ssl_handler()->on_ssl_write(ssl, buf, num, ret, start_time, end_time);
}

