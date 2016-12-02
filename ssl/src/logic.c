#include "../logic.h"
//#include <malloc.h>
//#include <memory.h>
//#include <string.h>
#include "jni.h"
#//include "../util/vstl/L3/system/time.h"
//#include "C_sockethandler.h"


/*
写入日志文件
*/
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
static void flog(const char* log){

	FILE * fp = fopen("/sdcard/test.txt","a+");
	if(NULL == fp){
	return;
	}

	fwrite(log,1,strlen(log),fp);
	fflush(fp);
	fclose(fp);
	fp = NULL;
}

#include <time.h>
#include <sys/time.h>

UINT64 nb_getSysTime(){
	struct timeval tv;
        gettimeofday(&tv, NULL);
	UINT64 tm = (tv.tv_sec) * 1000000 + tv.tv_usec;
	char buf[64];
    	snprintf(buf, sizeof(buf), "cccc---%u\n", tm);
	flog(buf);
	onMessage(buf);
        return tm;
}

void nb_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time){
//    LOGD("nb_ssl_create");
    get_ssl_handler()->on_ssl_create(ctx, ret, start_time, end_time);
}
/*
void nb_ssl_close(void *ssl, UINT64  start_time){
//    LOGD("nb_ssl_close");
    get_ssl_handler()->on_ssl_close(ssl, start_time);
}
void nb_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd){
//    LOGD("nb_ssl_connect");
    get_ssl_handler()->on_ssl_connect(ssl,ret,start_time,end_time,fd);
}

void nb_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
//    LOGD("nb_ssl_read");
    get_ssl_handler()->on_ssl_read(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
//    LOGD("nb_ssl_write");
    get_ssl_handler()->on_ssl_write(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time){
//    LOGD("nb_ssl_set_fd");
    get_ssl_handler()->on_ssl_set_fd(s, fd, ret, start_time);
}*/
