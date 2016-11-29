#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#endif
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <stdio.h>
namespace system_time3 {

    static char *time_to_string(time_t time_sec, char *time_str)
    {
        struct tm time_struct = *localtime(&time_sec);
        sprintf(time_str, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
        return time_str;
    }

#ifdef _WIN32

    static
        int get_system_time(timeval *_val) 
    {
        //11644473600: 1600.01.01 00:00:00~ 1970.01.01 00:00:00 经过的秒数
        SYSTEMTIME curr_Time;
        ::GetSystemTime(&curr_Time);
        FILETIME file_time;
        ::SystemTimeToFileTime(&curr_Time, &file_time);
        UINT64 *sys_time = (UINT64*)&file_time;
        UINT64 top_time = (*sys_time) / 10;
        _val->tv_sec = (DWORD)(top_time / 1000000 - 11644473600 );
        _val->tv_usec = (DWORD)(top_time % 1000000);
        return 0;
    }
    static
        int set_system_time(const timeval *_val) 
    {
        FILETIME file_time;
        UINT64 *sys_time = (UINT64*)&file_time;
        UINT64 top_time = _val->tv_sec;
        top_time += 11644473600;
        top_time *= 1000000;
        top_time += _val->tv_usec;
        *sys_time = top_time * 10;

        SYSTEMTIME curr_Time;
        if ( ! ::FileTimeToSystemTime(&file_time, &curr_Time) ) return -1;
        if ( ::SetSystemTime(&curr_Time) ) return 0;
        return -1;
    }
#else
    static 
        int get_system_time(timeval *_val) 
    {
        return gettimeofday(_val, NULL);
    }

    static
        int set_system_time(const timeval *_val) 
    {
        return settimeofday(_val, NULL);
    }

#endif

    static long long GetTickCount64()
    {
        timeval tv;
        get_system_time(&tv);
        return ((long long)tv.tv_sec) * 1000000 + tv.tv_usec;
    }
}
