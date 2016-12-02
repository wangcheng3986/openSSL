//
// Created by Administrator on 2016/12/2.
//

#include "cpluswrapper.h"
#include "hook_interface.h"
using namespace ssl_hook;

void on_ssl_create(void *ctx, void *ret, long long start_time, long long end_time){
    ssl_hook::ssl_handler*  handler = get_ssl_handler();
    handler->on_ssl_create(ctx, ret, start_time, end_time);
}