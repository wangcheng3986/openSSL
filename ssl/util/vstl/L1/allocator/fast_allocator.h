#pragma once
#include "../../L0/abstract/allocator.h"
namespace fast_allocator_l1 {
    class fast_allocator :public allocator_l0::allocator {

        int _full_size;
        int _used;
        char *base() { return (char*)this + sizeof(*this); }
    public:
        fast_allocator(int size):_full_size(size - sizeof(fast_allocator)), _used(0){}
        virtual ~fast_allocator(){}
        virtual void *allocate(size_t size) { 
            if ( _full_size - _used < (int)size ) return 0;
            void *ret = (void *) (base() + _used);
            _used += (int)size;
            return ret;
        }
        virtual void deallocate(void *_ptr) {}
        void reset() { _used = 0; }
    };

}