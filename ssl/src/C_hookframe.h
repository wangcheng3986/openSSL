#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

#include "C_sockethandler.h"
#include "../util/vstl/L3/system/thread.h"
class C_hookframe : public thread_l3::service_base, public C_sockethandler {
    virtual void on_task(const char *);

    virtual bool process();

public:
    C_hookframe(void);

    ~C_hookframe(void);

    virtual void destroy();

private:
    thread_l3::service_frame _thread_frame;
};

extern "C" C_hookframe *get_ssl_handler();

extern "C" void c_sslcreate(C_hookframe *p, void *ctx, void *ret, UINT64 start_time,
                            UINT64 end_time) // wrapper function
{
    return p->on_ssl_create(ctx, ret, start_time, end_time);
}

#ifdef  __cplusplus
}
#endif