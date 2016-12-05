#pragma once

//#include <bits/atomicity.h>
//#include <ext/atomicity.h> //ubuntu
//#include <c++/ext/atomicity.h>

static
long interlockedExchangeAdd(volatile long *_target, long _add_val) 
{
    return 0;//__gnu_cxx::__exchange_and_add((volatile _Atomic_word*)_target, _add_val);
}

//#pragma pack(pop)
