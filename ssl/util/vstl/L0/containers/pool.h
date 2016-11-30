#pragma once
#include "../abstract/type.h"
type_l0::dword_t xadd(volatile type_l0::dword_t*word, type_l0::dword_t add);
namespace container_l0 {
    using namespace type_l0;
    class pool_node {
        pool_node *_next;
    public:
        pool_node():_next(0){}
        void init(){_next = 0;}
        pool_node *x_next(pool_node *next) {
            pool_node *retval = _next; _next = next; return retval;
        }
    };

    template <dword_t threads=128>
    class pool
    {
        class list {
            volatile dword_t _access;
            pool_node *_first;
            pool_node *_last;
        public:
            list():_access(0), _first(0), _last(0){}
            bool put( pool_node *obj ) {
                if ( xadd(&_access, 1) != 0 ) { xadd(&_access, -1); return false; }
                obj->x_next(0);
                if ( _last ) _last->x_next(obj);
                _last = obj;
                if ( _first == 0 ) _first = obj;
                xadd(&_access, -1); return true;
            }
            pool_node * get() {
                if ( xadd(&_access, 1) != 0 ) { xadd(&_access, -1); return 0; }
                pool_node *retval = _first;
                if ( retval ) {
                    _first = retval->x_next(0);
                    if ( _first == 0 ) _last = 0;
                }
                xadd(&_access, -1); return retval;
            }
        };

        dword_t _count;
        dword_t _w_index;
        dword_t _r_index;
        list _list[threads];
    public:
        pool()
            : _r_index(0)
            , _w_index(0)
            , _count(0){}
    public:
        ~pool(void){}
    public:

        void put( pool_node *obj ){
            while ( true ) {
                for ( dword_t i = _w_index; i - _w_index < threads; i++ ) {
                    dword_t top_id = i % threads;
                    if ( _list[top_id].put(obj) ) {
                        _w_index = top_id + 1;
                        xadd(&_count, 1); return;
                    }
                }
            }
        }
    public:
        pool_node* get(){
            for ( dword_t i = _r_index; ( _count > 0 ) && ( i - _r_index < threads ); i++ ) {
                dword_t top_id = i % threads;
                pool_node *node = _list[top_id].get();
                if ( node ) {
                    xadd(&_count, -1);
                    _r_index = top_id + 1;
                    return node;
                }
            }
            return 0;
        }
    public:
        dword_t size() const { return _count; }
    };

}
