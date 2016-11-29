#pragma once
#include "../common.h"
#include "../../L0/containers/list.h"
namespace container_l2 {

    template <class T>
    class list :public allocator_l0::contain_base {
        struct node :public container_l0::list::node {
            T _value;
            node(const T &val, allocator_l0::allocator *Allocate) {
                ::set_allocator(_value, Allocate);
                _value = val;
            }
            ~node(){}
        };
        container_l0::list _base;
    public:
        typedef list<T> my_type;

        class iterator {
        protected:
            friend class list<T>;
            container_l0::list::node *_node;
        public:
            iterator():_node(0){}
            iterator(container_l0::list::node *base_node):_node(base_node){}
            iterator(const iterator &right) :_node(right._node){}
            iterator &operator = (const iterator &right) {
                _node = right._node;
            }
            iterator &operator ++ () {
                if ( _node ) _node = _node->_next;
                return *this;
            }
            iterator &operator -- () {
                if ( _node ) _node = _node->_pre;
                return *this;
            }
            T &operator * () {
                assert(_node);
                return (static_cast<node *>(_node))->_value;
            }
            bool operator == (const iterator &right) {
                return _node == right._node;
            }
            bool operator != (const iterator &right) {
                return ! operator == (right);
            }
        };

        iterator end() const { return iterator(); }
        iterator begin() const { return iterator(_base.front()); }
        iterator rbegin() const { return iterator(_base.back()); }

        list(const my_type &rval) :contain_base(rval.get_allocator()) {
            for ( iterator it  = rval.begin(); it != rval.end(); ++it ) {
                push_back ( *it );
            }
        }
        ~list() { clear(); }
        my_type &operator = (const my_type &rval) {
            for ( iterator it  = rval.begin(); it != rval.end(); ++it ) {
                push_back ( *it );
            }
            return *this;
        }
        list() {}
        size_t size() const { return _base.size(); }

        bool push_back(const T &val) {
            allocator_l0::my_allocator<node> my_allocate(get_allocator());
            node *pnode = my_allocate.allocate(1);
            if ( pnode == 0 ) return false;
            new ((void*)pnode) node(val, get_allocator());
            _base.push_back(pnode);
            return true;
        }
        bool push_front(const T &val) {
            allocator_l0::my_allocator<node> my_allocate(get_allocator());
            node *pnode = my_allocate.allocate(1);
            if ( pnode == 0 ) return false;
            new ((void*)pnode) node(val, get_allocator());
            _base.push_front(pnode);
            return true;
        }
        void pop_front() {
            container_l0::list::node *base_node = _base.pop_front();
            if ( base_node ) {
                node *pnode = static_cast<node *>(base_node);
                pnode->~node();
                allocator_l0::my_allocator<node> my_allocate(get_allocator());
                my_allocate.deallocate(pnode, 1);
            }
        }
        void pop_back() {
            container_l0::list::node *base_node = _base.pop_back();
            if ( base_node ) {
                node *pnode = static_cast<node *>(base_node);
                pnode->~node();
                allocator_l0::my_allocator<node> my_allocate(get_allocator());
                my_allocate.deallocate(pnode, 1);
            }
        }
        T & front() {
            container_l0::list::node *base_node = _base.front();
            assert(base_node);
            node *pnode = static_cast<node *>(base_node);
            return pnode->_value;
        }
        T & back() {
            container_l0::list::node *base_node = _base.back();
            assert(base_node);
            node *pnode = static_cast<node *>(base_node);
            return pnode->_value;
        }
        iterator erase(iterator it) { 
            iterator ret = it;
            ret++;
            if ( it._node ) {
                _base.erase(it._node);
                node *pnode = static_cast<node *>(it._node);
                pnode->~node();
                allocator_l0::my_allocator<node> my_allocate(get_allocator());
                my_allocate.deallocate(pnode, 1);
            }
            return ret;
        }
        void clear() {
            while ( size() ) pop_front();
        }
    };
}
