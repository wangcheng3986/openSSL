#pragma once

#ifdef  __cplusplus
extern "C" {
#endif
namespace hook_callback {
    class logic_handler {
    public:
        virtual void on_task(const char *) = 0;
    };
}

#ifdef  __cplusplus
}
#endif