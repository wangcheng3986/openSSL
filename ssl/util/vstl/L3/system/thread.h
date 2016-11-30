#pragma once
#ifdef _WIN32
#include <process.h>
#include <Windows.h>
#else 
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
static
void Sleep(unsigned int ms) {
    usleep(ms * 1000);
}
#endif
#include "../../L0/abstract/type.h"
namespace thread_l3 {
    using namespace type_l0;
    class thread_base {
    public:
        virtual void main() = 0;
    };
    static
#ifdef _WIN32 
        UINT WINAPI 
#else
        void *
#endif
        proxy_proc(void *obj)
    {
        thread_base *thread_obj = (thread_base*)obj;
        thread_obj->main();
        return 0;
    }

#ifdef WIN32
    static
        unsigned int start_thread( thread_base *thread_obj, unsigned int stackSize)
    {
        unsigned int thread_id = 0;
        HANDLE h_thread = (HANDLE) _beginthreadex(NULL, stackSize, &proxy_proc, (void*)thread_obj, 0, &thread_id);
        if ( h_thread ) CloseHandle((HANDLE)h_thread);
        return thread_id;
    }


#else

    static
        pthread_t start_thread(thread_base *thread_obj, unsigned int stackSize)
    {

        pthread_t threadPoint = 0;

        pthread_attr_t theThreadAttr;
        pthread_attr_init(&theThreadAttr);

        if ( stackSize ) pthread_attr_setstacksize(&theThreadAttr, stackSize);
        pthread_attr_setdetachstate(&theThreadAttr, PTHREAD_CREATE_DETACHED);

        if ( pthread_create ( &threadPoint, &theThreadAttr, &proxy_proc, (void*)thread_obj ) ) threadPoint = 0;
        pthread_attr_destroy(&theThreadAttr);
        return threadPoint;
    }

#endif


    class service_base {
    public:
        virtual bool process() = 0;
        virtual bool running() { return true; }
    };

    class service_frame :public thread_base {
        service_base *_service;
        volatile bool _running;
        volatile bool _loaded;
        dword_t _thread_count;
        dword_t _stack_size;
        dword_t _wait_msec;
    public:
        service_frame(service_base *svc, int stack_size = 2 * 1024 * 1024)
            :_wait_msec(1), _service(svc), _thread_count(0), _running(false), _stack_size(stack_size), _clear_obj(0), _loaded(false){}
        virtual ~service_frame(){stop();}
    public:
        class clear_obj {
        public:
            virtual void clear() = 0;
        };
        void stop(clear_obj *clear_obj = 0) {
            _clear_obj = clear_obj;
            _running = false;
            while ( _thread_count > 0 ) Sleep(1);
        }
        bool start(int wait_msec = 1, bool wait_thread = true) {
            _wait_msec = wait_msec;
            if ( _service == 0 ) return false;
            _running = true;
            bool ret = start_thread(this, _stack_size) != 0;
            if ( ret && wait_thread ) {
                for ( int i = 0; i < 10000; i++ ) {
                    if ( _loaded ) break;
                    else Sleep(1);
                }
            }
            return ret;
        }
    private:
        clear_obj *_clear_obj;
        virtual void main() {

            _thread_count++;
            _loaded = true;
            while ( _running ) {
                if ( ! _service->process() ) Sleep(_wait_msec);
            }
            if ( _clear_obj ) _clear_obj->clear();
            _thread_count--;
        }
    };

}