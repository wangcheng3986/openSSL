//放置内置类型的别名
#pragma once
namespace type_l0 {
    typedef unsigned char byte_t;
    typedef unsigned short word_t;
//#if (sizeof(unsigned int) == 4)
    typedef unsigned int dword_t;
//#else if ( sizeof(long) == 4 ) 
//    typedef unsigned long dword_t;
//#else if ( sizeof(long int) == 4 ) 
//    typedef unsigned long int dword_t;
//#endif

//#if (sizeof(long long) == 8)

    typedef unsigned long long qword_t;
//#endif
}
#ifndef _WIN32
#ifndef UINT64
#define UINT64 unsigned long long
#endif
#endif