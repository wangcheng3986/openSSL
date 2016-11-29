#pragma once
typedef void * void_pointer;

class Tls {
    void *_threads[0x10000];
public:
    Tls() {init();}
    void init() { for ( int i = 0; i < sizeof(_threads) / sizeof(_threads[0]); i++ ) _threads[i] = 0; }
    void_pointer &getThreadLocal() 
    {
        return _threads[(unsigned short int)gettid()];
    }
    void releaseThreadLocal()
    {
        _threads[(unsigned short int)gettid()] = 0;
    }
    void clear() 
    {
        for ( int i = 0; i < sizeof(_threads) / sizeof(_threads[0]); i++ ) _threads[i] = 0;
    }
};