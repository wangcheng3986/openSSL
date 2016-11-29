#pragma once
namespace hook_callback {
    class logic_handler {
    public:
        virtual void on_task(const char *) = 0;
    };
}