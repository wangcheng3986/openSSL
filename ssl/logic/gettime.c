//
// Created by Administrator on 2016/12/8.
//

#include "gettime.h"
#include "log.h"

#include <time.h>
#include <sys/time.h>
#include <stddef.h>

long getSysTime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long tm = (tv.tv_sec) * 1000 + tv.tv_usec/1000;
    char buf[256];
    sprintf(buf, "--------------------%ld--%ld,%ld", tm,  tv.tv_sec,tv.tv_usec);
    flog(buf);
    return tm;
}