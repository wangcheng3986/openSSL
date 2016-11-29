#pragma once
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include "../util/vstl/L0/abstract/unistd.h"
#include "../util/vstl/L2/buffer/buffer.h"
#include "../util/vstl/L2/containers/map.h"
#include "../util/vstl/L2/containers/string.h"
#include "../util/vstl/L3/system/time.h"
#include "../util/vstl/L3/system/atom.h"
#include "../util/vstl/L3/io/file.h"

class log_writer {
public:
    virtual void destroy() = 0;
    virtual int append(const char *buf, int len) = 0;
    virtual int printf(const char* lpszFormat, ...) = 0;
    virtual void write_time() = 0;
};
log_writer *create_error_log();
log_writer *create_data_log();

#ifdef _WIN32 
class C_pthreadMutex {
public:
    C_pthreadMutex() { }
    ~C_pthreadMutex(){  }
    bool lock() { return true; }
    void unlock() { }
};
struct rlimit {
    int rlim_cur;  /* Soft limit */
    int rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};
static 
int getrlimit(int resource, struct rlimit *rlim)
{
    return 0;
}
#define RLIMIT_NOFILE 1
#else
class C_pthreadMutex {
    pthread_mutex_t _mutex;
public:
    C_pthreadMutex() {  pthread_mutex_init(&_mutex, 0); }
    ~C_pthreadMutex(){ pthread_mutex_destroy(&_mutex); }
    bool lock() { return pthread_mutex_lock(&_mutex) == 0; }
    void unlock() { pthread_mutex_unlock(&_mutex); }
};
#endif
class auto_lock {
    C_pthreadMutex &_lock;
    auto_lock(const auto_lock &);
public:
    auto_lock(C_pthreadMutex &lock): _lock(lock) { _lock.lock(); }
    ~auto_lock() { _lock.unlock(); }
};

class fd_atom {
    long *_locks;
    int _count;
public:
    fd_atom() {
        struct rlimit fd_limit = {1024, 1024};
        getrlimit(RLIMIT_NOFILE, &fd_limit);
        _count = unistd_l0::Min<int>(fd_limit.rlim_cur, fd_limit.rlim_max);
        _count = unistd_l0::Max(1024, _count);
        _locks = new long[_count];
        for ( int i = 0; i < _count; i++ ) _locks[i] = 0;
    }
    ~fd_atom() { delete[] _locks; _locks = 0; }
    void fd_open(int fd) {
        assert(fd < _count);
        long ret = interlockedExchangeAdd(&(_locks[fd]), 1);
        assert(ret == 0);
    }
    bool fd_close(int fd) {
        if ( fd < 0 ) return false;
        assert(fd < _count);
        if ( interlockedExchangeAdd(&(_locks[fd]), -1) == 1 ) return true;
        interlockedExchangeAdd(&(_locks[fd]), 1);
        return false;
    }
    bool fd_used(int fd) {
        if ( fd < 0 ) return false;
        assert(fd < _count);
			
		if(fd >= _count){
			return false;
		}
        return _locks[fd] == 1;
    }
};
