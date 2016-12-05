//
// Created by Administrator on 2016/12/2.
//

#include "cpluswrapper.h"
#include "C_hookframe.h"


struct C_hookframe *get_ssl_handler();
void c_sslcreate(struct  C_hookframe* p, void *ctx, void *ret, UINT64 start_time, UINT64 end_time);


void on_ssl_create(void *ctx, void *ret, long long start_time, long long end_time) // wrapper function
{
    struct C_hookframe* p = get_ssl_handler();
    c_sslcreate(p, ctx,ret,start_time,end_time);
}