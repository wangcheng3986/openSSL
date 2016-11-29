#pragma once
//#pragma pack(push, 1)

#ifdef WIN32

#include <Windows.h>
#define interlockedExchangeAdd InterlockedExchangeAdd

#else

#ifdef __IOS_
#include <libkern/OSAtomic.h>
static
long interlockedExchangeAdd(volatile long *_target, long _add_val) 
{
    return OSAtomicAdd32((int32_t)_add_val, (volatile int32_t *)_target) - _add_val;
}
#else
//#include <bits/atomicity.h>
#include <ext/atomicity.h> //ubuntu
static
long interlockedExchangeAdd(volatile long *_target, long _add_val) 
{
    return __gnu_cxx::__exchange_and_add((volatile _Atomic_word*)_target, _add_val);
}
#endif

#endif

//#pragma pack(pop)
