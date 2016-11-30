#pragma once
#include "../../L0/abstract/allocator.h"
namespace container_l2 {

    template <typename T, const int default_size> class smart_buffer : public allocator_l0::contain_base {
        int _count;
        T _cache[default_size];
        T *_new_buf;
    public:
        smart_buffer(int count = default_size):_count(count), allocator_l0::contain_base(0){
            allocator_l0::default_allocator tmp_alloc(_allocator);
            _new_buf = (count <= default_size)? _cache : (T *)tmp_alloc.allocate(sizeof(T) * count);
        }
        smart_buffer(allocator_l0::allocator *Allocate, int count = default_size):_count(count), allocator_l0::contain_base(Allocate){

            allocator_l0::default_allocator tmp_alloc(_allocator);
            _new_buf = (count <= default_size)? _cache : (T *)tmp_alloc.allocate(sizeof(T) * count);
        }
        ~smart_buffer(){
            if ( _new_buf != _cache ) {
                allocator_l0::default_allocator tmp_alloc(_allocator);
                tmp_alloc.deallocate(_new_buf); 
            }
        }
        T *buffer() { return _new_buf; }
        T &operator [] (int index) { return _new_buf[index]; }

        int count() const { return _count; }
    };

}