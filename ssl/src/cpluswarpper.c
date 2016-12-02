//
// Created by Administrator on 2016/12/2.
//

#include "cpluswrapper.h"
#include "hook_interface.h"

extern "C" void on_ssl_create(void *ctx, void *ret, long long start_time, long long end_time) // wrapper function
{
    return get_ssl_handler()->on_ssl_create(ctx,ret,start_time,end_time);
}