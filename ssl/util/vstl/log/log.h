#pragma once
#include "../L0/abstract/allocator.h"
#include <string.h>
#ifdef _WIN32
#pragma warning (disable : 4996)
#ifndef strcasecmp
#define strcasecmp stricmp
#endif
#endif
namespace log_writer {
    enum log_level {
        log_off = 0,
        log_critical = 1,
        log_error = 2,
        log_warning = 3,
        log_info = 4,
        log_verbose = 5,
        log_debug = 6,
        log_undef = 7
    };
    static
    log_level level_num(const char *value) {
        if ( strcasecmp(value, "off") == 0 ) return log_off;
        if ( strcasecmp(value, "critical") == 0 ) return log_critical;
        if ( strcasecmp(value, "error") == 0 ) return log_error;
        if ( strcasecmp(value, "warning") == 0 ) return log_warning;
        if ( strcasecmp(value, "info") == 0 ) return log_info;
        if ( strcasecmp(value, "verbose") == 0 ) return log_verbose;
        if ( strcasecmp(value, "debug") == 0 ) return log_debug;
        return log_undef;
    }
    class log_writer {
    public:
        virtual void destroy() = 0;
        //log,只写的
        virtual bool write(log_level level, const char *data, int len) = 0;
        //设置,日志大小，文件个数,level
        virtual const char *ctrl(const char *cmd, char *data, int len) = 0;
        virtual int printf(log_level level, const char* format_desc, ...) = 0;
        virtual bool append(log_level level, const char *data, int len) = 0;
        virtual bool finished() = 0;
    };
    class log_creater {
    public:
        virtual bool try_destroy() = 0;
        virtual log_writer *create(const char *filename) = 0;
        virtual bool loop_check() = 0;
        static log_creater *create(allocator_l0::allocator *Allocate);
    };
}