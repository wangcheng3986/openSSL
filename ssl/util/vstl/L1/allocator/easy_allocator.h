#pragma once
#include "../../L0/abstract/allocator.h"
namespace allocator_l1 {
    struct alloc_block;
    struct alloc_unit {
        alloc_block *_block;
        void *data() {
            return (void*) ((char *)this + sizeof(*this));
        }
        static alloc_unit *inst_by_member(void *p) {
            return (alloc_unit *)((char*)p - sizeof(alloc_unit));
        }
    };
    struct alloc_block {
        struct list_node {
            list_node *_next;
        };
        alloc_block *_next;
        alloc_block *_pre;
        list_node *_unit_list;//回收链表
        unsigned short _size;//单元个数
        unsigned short _unit;//单元大小
        unsigned short _used;//已经分配的数量
        unsigned short _access;//访问过的
        void remove() {
            _pre->_next = _next;
            if ( _next ) _next->_pre = _pre;
        }
        void insert(alloc_block *blist) {
            _next = blist->_next;
            _pre = blist;
            _pre->_next = this;
            if ( _next ) _next->_pre = this;
        }
        bool can_alloc() const { return _used < _size; }
        char *begin() {
            return  (char *)this + sizeof(*this);
        }
        void init(int block_size, int node_size) {
            int can_used = block_size - sizeof(*this);
            _unit_list = 0;
            _unit = node_size;
            _used = 0;
            _access = 0;
            _size = can_used / node_size;
        }
        alloc_unit *get_node() {
            if (_unit_list == 0) return 0;
            alloc_unit *ret = (alloc_unit*)_unit_list;
            _unit_list = _unit_list->_next;
            return ret;
        }
        void *alloc() {
            if ( ! can_alloc() ) return 0;
            alloc_unit *node = get_node();
            if ( node == 0 ) {
                node = (alloc_unit *)(begin() + _access * _unit);
                _access++;
            }
            node->_block = this;
            _used++;
            return node->data();
        }
        bool free_mem(void *ptr) {
            assert( (ptr >= (void *)begin()) && (ptr <= (void*)(begin() + (_access - 1) * _unit)) );
            list_node *node = (list_node*)ptr;
            node->_next = _unit_list;
            _unit_list = node;
            _used--;
            return true;
        }
    };
    struct alloc_node {
        alloc_block *_alloc_list;
        alloc_block *_empty_list;
        unsigned short _node_size;
        alloc_node():_alloc_list(0), _empty_list(0){}
        void insert_empty(alloc_block *block) {
            block->insert((alloc_block*)&_empty_list);
        }
        void insert_used(alloc_block *block) {
            block->insert((alloc_block*)&_alloc_list);
        }
        void init( int node_size) {
            _alloc_list = 0;
            _empty_list = 0;
            _node_size = node_size;
        }
    };
    class Policy {
    public:
        virtual int alloc_value(int size) = 0;
        virtual unsigned short alloc_size(int id) = 0;
    };
    template <int alloc_count, int block_size>
    class allocator :public allocator_l0::allocator {
        alloc_node _alloc_array[alloc_count];
        int _size;
        alloc_block *_top_block;
        Policy *_policy;
        int _use_count;
        int _use_size;
        void init() {

            for ( int i = 0; i < alloc_count; i++ ) {
                _alloc_array[i].init(_policy->alloc_size(i));
            }
            _top_block->init(_size, block_size);
            _top_block->_pre = _top_block->_next = 0;
        }
    public:
        allocator(char *buffer, int size, Policy *policy):_top_block((alloc_block*)buffer), _size(size), _policy(policy),_use_size(0),_use_count(0) {
            init();
        }
        void *alloc(int size) {
            int rsize = size + sizeof(alloc_unit);
            alloc_node *node = get_node(rsize);
            if ( node == 0 ) {
                alloc_unit *ret = (alloc_unit *)::malloc(rsize);
                ret->_block = 0;
                return ret->data();
            }
            alloc_block *block = node->_alloc_list;
            if ( block == 0 ) {
                block = (alloc_block*)_top_block->alloc();
                if ( block == 0 ) return 0;
                block->init(block_size - sizeof(alloc_unit), node->_node_size);
                node->insert_used(block);
            }
            void *ret = block->alloc();
            if ( ! block->can_alloc() ) {
                block->remove();
                node->insert_empty(block);
            }
            return ret;
        }
        alloc_node *get_node(int size) { 
            int id = _policy->alloc_value(size);
            if ( id == -1 ) return 0;
            return &(_alloc_array[id]); 
        }

        virtual void *allocate(size_t size) { 
            void *ret = alloc((int)size);
            if ( ret ) {
                _use_count++;
                _use_size += (int)size;
            }
            return ret;
        }
        virtual void deallocate(void *ptr) {
            free_mem(ptr);
            _use_count--;
        }
        void free_mem(void *ptr) {

            alloc_unit *top_unit = alloc_unit::inst_by_member(ptr);
            alloc_block *top_block = top_unit->_block;
            if ( top_block == 0 ) {
                ::free(top_unit); return;
            }
            else if ( top_block == _top_block ) {
                _top_block->free_mem(top_unit);
                return;
            }
            bool is_empty = ! top_block->can_alloc();
            top_block->free_mem(top_unit);
            if ( is_empty || top_block->_used == 0 ) {
                top_block->remove();
                if ( is_empty ) {
                    alloc_node *node = get_node(top_block->_unit);
                    node->insert_used(top_block);
                }
                else if ( top_block->_used == 0 ) {
                    free_mem (top_block);
                }
            }
        }
        void print() {
            printf("use count = %d use size = %d\n", _use_count, _use_size);
        }
        void reset() {
            _use_size = _use_count = 0;
            init();
        }
    };

    template <int count>
    class MyPolicy :public Policy {

        unsigned short _list[count + 1];
    public:
        MyPolicy (unsigned short *alloc_array) {
            for ( int i = 0; i < count; i++ ) _list[i] = alloc_array[i];
            _list[count] = 0;
        }

        virtual int alloc_value(int size) {

            if ( size < 0 || size > _list[count - 1] ) return -1;
            int step = (count + 1) / 2;
            int id = count / 2;
            while ( step ) {
                step /= 2;
                id += (_list[id] < size) ? step: - step;
            }
            while ( _list[id] < size ) id++;
            while ( id && ( size <= _list[id - 1] ) ) id--;
            return id;
        }
        virtual unsigned short alloc_size(int id) { return _list[id]; }
    };

}