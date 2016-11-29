#pragma once
#ifdef _WIN32
#ifndef HANDLE
#include <Windows.h>
#endif
#endif
#include <stdio.h>
namespace system_dll3 {

#ifdef _WIN32
    class dll_loader {
        HANDLE _hdll;
    public:
        dll_loader(): _hdll(0){}
        ~dll_loader(){this->unload();}
        bool load(const char *dll_path) {
            if ( _hdll == 0 ) _hdll = LoadLibraryA(dll_path);
            return _hdll != 0;
        }
        void unload() {
            if ( _hdll ) FreeLibrary((HMODULE)_hdll);
            _hdll = 0;
        }
        void *query_proc(const char *_proc_name) {

            if ( _hdll ) return (void*)GetProcAddress((HMODULE)_hdll, _proc_name);
            return 0;
        }
    };
#else

#include <dlfcn.h>
    class dll_loader {
        void * _hdll;
    public:
        dll_loader():_hdll(0){}
        ~dll_loader(){unload();}
        bool load(const char *dll_path) {
            if ( _hdll == 0 ) _hdll = dlopen(dll_path, RTLD_NOW);
            if ( _hdll == 0 ) printf("\ndll (%s) load error:%s\n",dll_path, dlerror());
            else printf("dlopen(%s) ==> %p\n", dll_path, _hdll);
            return _hdll != 0;
        }
        void unload() {
            if ( _hdll ) {
                dlclose(_hdll);
                printf("dlclose(%p)\n", _hdll);
            }

            _hdll = 0;
        }
        void *query_proc(const char *_proc_name) {

            if ( _hdll ) return (void*)dlsym(_hdll, _proc_name);
            return 0;
        }
    };
#endif

}
