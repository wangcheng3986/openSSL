#include "C_hookframe.h"
#include "jnilog.h"

char *get_retbuf();

C_hookframe::C_hookframe(void)
: _thread_frame(this)
{
    _thread_frame.start();
}

C_hookframe::~C_hookframe(void)
{
    _thread_frame.stop();
}

bool report_log(const char *log);
void C_hookframe::on_task(const char *log)
{
    LOGD("on_task:%s",log);
}



bool C_hookframe::process()
{
    run();
    return false;
}

static C_hookframe *inst = 0;

ssl_hook::ssl_handler *get_ssl_handler()
{
    if ( inst == 0 ) inst = new C_hookframe;
        return inst;
}

void C_hookframe::destroy() 
{
    assert(inst == this);
    delete this; 
    inst = 0;
}

