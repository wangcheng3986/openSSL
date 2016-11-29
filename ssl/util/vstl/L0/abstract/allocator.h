#pragma once
#include <stdio.h>
#include "op_new.h"
#ifdef _DEBUG
//#include <typeinfo>
#endif
namespace allocator_l0 {
    class allocator {
    public:
        virtual void *allocate(size_t size) = 0;
        virtual void deallocate(void *) = 0;
    };
 
    class default_allocator :public allocator {
        allocator *_allocator;
    public:
        default_allocator():_allocator(0){ }
        default_allocator(allocator *Allocate):_allocator(Allocate){ }
        virtual ~default_allocator(){ }
    public:
        virtual void *allocate(size_t _size) { 
            
            return _allocator? _allocator->allocate(_size) :  malloc(_size);
        }
        virtual void deallocate(void *ptr) {
            if ( _allocator ) _allocator->deallocate(ptr);
            else free(ptr);
        }
    };

    template<class _Ty>
    class my_allocator
    {
        allocator *_allocator;
    public:
        typedef _Ty value_type;
        typedef value_type  *pointer;
        typedef value_type & reference;
        typedef const value_type  *const_pointer;
        typedef const value_type & const_reference;

        typedef size_t size_type;
        typedef int difference_type;
        template<class _Other>
        struct rebind
        {
            typedef my_allocator<_Other> other;
        };

        pointer address(reference val) const { return (&val); }

        const_pointer address(const_reference val) const { return (&val); }

        my_allocator() throw ()
            :_allocator(0)
        {
        }

        my_allocator(allocator *Allocator) throw ()
            :_allocator(Allocator)
        {
        }

        my_allocator(const my_allocator<value_type>&val) throw ()
            :_allocator(val._allocator)
        {
        }
        ~my_allocator(){
        }
        allocator *native_inst() const { return _allocator; }

        template<class _Other>
        my_allocator(const my_allocator<_Other>&_val) throw ()
            :_allocator(_val.native_inst())
        {
        }

        template<class _Other>
        my_allocator<value_type>& operator=(const my_allocator<_Other>&val)
        {
            _allocator = val.native_inst();
            return (*this);
        }

        template<class _Other>
        bool operator == (const my_allocator<_Other>&val)
        {
            return _allocator == val._allocator;
        }

        template<class _Other>
        bool operator != (const my_allocator<_Other>&val)
        {
            return _allocator != val._allocator;
        }

        void deallocate(pointer ptr, size_type)
        {
            if ( _allocator ) _allocator->deallocate(ptr);
            else free((void*)ptr);
        }

        pointer allocate(size_type count)
        {
            if ( _allocator == 0 ) {
//                printf("no allocator, alloc %d\n", count);
            }
            if ( _allocator ) return pointer(_allocator->allocate(count * sizeof(value_type)));
            return pointer(  malloc( count * sizeof(value_type) )  );
        }

        pointer allocate(size_type count, const void  *)
        {
            return allocate(count);
        }

        void construct(pointer ptr, const value_type& val)
        {
            ::new ((void*)ptr) value_type(val);
        }

        void destroy(pointer ptr)
        {
            ptr->~value_type();
        }

        size_type max_size() const throw ()
        {
            size_type count = (size_type)(-1) / sizeof (value_type);
            return (0 < count ? count : 1);
        }
    };

    class contain_base {
    public:
        contain_base():_allocator(0){}
        ~contain_base(){}
        contain_base(allocator *alloc):_allocator(alloc){}
        void set_allocator(allocator *alloc) { _allocator = alloc; }
        allocator *get_allocator() const { return _allocator; }
    protected:
        allocator *_allocator;
    };
}

static void set_allocator(allocator_l0::contain_base &val, allocator_l0::allocator *Allocate) {
    val.set_allocator(Allocate);
}
template<class T>
static void set_allocator(T &val, allocator_l0::allocator *) {
#ifdef _DEBUG
//    printf("set_allocator(unknown type %s)\n", typeid(val).name());
#endif
}

