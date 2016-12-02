#include "../logic.h"
#include "log.h"
#include <time.h>
#include <sys/time.h>

UINT64 nb_getSysTime(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	UINT64 tm = (tv.tv_sec) * 1000000 + tv.tv_usec;
	char buf[64];
	snprintf(buf, sizeof(buf), "nb_getSysTime---%u\n", tm);
	flog(buf);
	return tm;
}


void nb_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_create\n");
   // get_ssl_handler()->on_ssl_create(ctx, ret, start_time, end_time);
}

void nb_ssl_close(void *ssl, UINT64  start_time){
	flog("nb_ssl_close\n");
//    get_ssl_handler()->on_ssl_close(ssl, start_time);
}
void nb_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_connect\n");
//    LOGD("nb_ssl_connect");
   // get_ssl_handler()->on_ssl_connect(ssl,ret,start_time,end_time,fd);
}

void nb_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_read\n");
//    LOGD("nb_ssl_read");
   // get_ssl_handler()->on_ssl_read(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
	flog("nb_ssl_write\n");
//    LOGD("nb_ssl_write");
   // get_ssl_handler()->on_ssl_write(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time){
	flog("nb_ssl_set_fd\n");
//    LOGD("nb_ssl_set_fd");
    //get_ssl_handler()->on_ssl_set_fd(s, fd, ret, start_time);
}
