#pragma once

#include "C_sockethandler.h"
#include "../util/vstl/L3/system/thread.h"
class C_hookframe :public thread_l3::service_base, public C_sockethandler
{
    virtual void on_task(const char *);
    virtual bool process();
public:
    C_hookframe(void);
    ~C_hookframe(void);
    virtual void destroy();
private:
    thread_l3::service_frame _thread_frame;
};
