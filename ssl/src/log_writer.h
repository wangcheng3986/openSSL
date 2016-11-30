#pragma once

#include <stdio.h>
#include <string.h>
#include "helper.h"
#include "../util/vstl/L3/protocol/http.h"
#include "../util/vstl/L3/system/time.h"

class my_log_writer :public log_writer{
    storage_l3::file_rw _file;
    virtual void destroy() { delete this; }
public:
    my_log_writer(const char *log_file):_file(log_file, O_WRONLY | O_CREAT | O_APPEND){}
    virtual int append(const char *buf, int len) {
        return _file.write(buf, len);
    }
    virtual int printf(const char* lpszFormat, ...)
    {
        if ( ! _file.fine() ) return -1;
        const int len = 1024 * 100;
        container_l2::smart_buffer<char, len> buffer;
        va_list args;
        va_start(args, lpszFormat);
        int size = vsnprintf(buffer.buffer(), len - 1, lpszFormat, args);
        va_end(args);
        _file.write(buffer.buffer(), size);
        return size;
    }
    virtual void write_time() {
        timeval log_time;
        system_time3::get_system_time(&log_time);
        char buffer[128];
        system_time3::time_to_string(log_time.tv_sec, buffer);
        int rsize = strlen(buffer);
        printf("\n%s.%.3d ", buffer, log_time.tv_usec / 1000);
    }
};

log_writer *create_error_log()
{
    return new my_log_writer("/sdcard/hookerror.log");
}
log_writer *create_data_log()
{
    return new my_log_writer("/sdcard/hooksock.log");
}