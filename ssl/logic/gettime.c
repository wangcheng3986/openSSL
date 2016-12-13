//
// Created by Administrator on 2016/12/8.
//

#include "gettime.h"
#include "log.h"

#include <time.h>
#include <sys/time.h>
#include <stddef.h>

int64 getSysTime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64 tm = ((int64)tv.tv_sec) * 1000000 + tv.tv_usec;

    char buf[256];
    sprintf(buf, "---tm-----------------%lld--%ld,%ld", tm,  tv.tv_sec,tv.tv_usec);
    flog(buf);
    return tm;
}