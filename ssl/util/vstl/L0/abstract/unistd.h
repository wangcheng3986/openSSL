#pragma once
#include <errno.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#include <io.h>
#else
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#endif
namespace unistd_l0 {
    template <typename T> 
    T Max ( T a, T b ) {
        return ( a < b ) ? b: a;
    }
    template <class Ty>
    Ty Min ( Ty a, Ty b ) {
        return ( a < b ) ? a: b;
    }
    static
    bool is_space(unsigned char ch) {
        return (unsigned char)(ch - 9) < 5 || ch == ' ';
    }
    static 
    const char *parse_digit_string ( const char *data, int &len) {
        while ( is_space(*data) ) data++;
        const char *retval = data;
        if ( *data == '-' ) data++;
        while ( (unsigned char)(*data - '0') < 10 ) data++;
        if ( *data == '.' ) {
            data++;
            const char *little_data =  data;
            while ( (unsigned char)(*data - '0') < 10 ) data++;
            //如果要求小数部分必须有数字，则打开下边注释
//            if ( little_data == data ) return 0;
        }
        // 'e' - 'E' = 0x20
        if ( ( *data | ('E' ^ 'e') ) == 'e' ) {
            data++;
            if ( *data == '-' || *data == '+' ) data++;
            while ( (unsigned char)(*data - '0') < 10 ) data++;
        }
        len = (int)(data - retval);
        return retval;
    }
    static long long atoi64(const char *str)
    {
        long long retval = 0;
        for ( ; *str; str++ ) {
            if ( (unsigned char)(*str - '0') > 9 ) break;
            retval = retval * 10 + (*str - '0');
        }
        return retval;
    }
    template <class T>
    static T atot(const char *str)
    {
        T retval = 0;
        for ( ; *str; str++ ) {
            if ( (unsigned char)(*str - '0') > 9 ) break;
            retval = retval * 10 + (*str - '0');
        }
        return retval;
    }

    static unsigned char _hex_val(unsigned char _ch)
    {
        if ( (unsigned char)(_ch - '0') < 10 ) return _ch - '0';
        if ( (unsigned char)( (_ch | ('a' ^ 'A')) - 'a') <= 'f' - 'a' ) return 10 + (_ch | ('a' ^ 'A')) - 'a';
        return 0xFF;
    }
    template <class T>
    static T atox(const char *_val)
    {
        T retval = 0;
        for (; *_val; _val++ ) {
            unsigned char hex_val = _hex_val(*_val);
            if ( hex_val == 0xFF ) return retval;
            retval = (retval << 4) + hex_val;
        }
        return retval;
    }

#ifndef MY_ERROR_NUM
#define MY_ERROR_NUM
    static
        int error_num()
    {
#ifdef _WIN32
        return GetLastError();
#else
        return errno;
#endif
    }
#endif


#ifdef _WIN32
    static int get_system_time(timeval *_val) 
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
#else
    static int get_system_time(timeval *_val) 
    {
        return gettimeofday(_val, NULL);
    }
#endif

}
