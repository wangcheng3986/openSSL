#pragma once
#include <assert.h>
namespace container_l0 {
    class list {
    public:
        struct node {
            node *_next;
            node *_pre;
            node *next() const { return _next; }
            node *pre() const { return _pre; }
            void insert_back(node *pre) {
                _pre = pre;
                _next = pre->_next;
                pre->_next = this;
                if ( _next ) _next->_pre = this;
            }
            void remove() {
                _pre->_next = _next;
                if ( _next ) _next->_pre = _pre;
            }
        };
    private:
        node *_left;
        node *_right;
        size_t _count;
    public:
        list():_left(0),_right(0),_count(0){}
        ~list(){}
        node *front() const {
            return _left;
        }
        node *back() const {
            return _right;
        }
        void push_back(node *new_node) {
            if ( _right == 0 ) push_front(new_node);
            else {
                new_node->insert_back(_right);
                _right = new_node;
                _count++;
            }
        }
        void push_front(node *new_node) {
            new_node->insert_back((node*)&_left);
            if ( _right == 0 ) _right = new_node;
            _count++;
        }
        node *pop_front() {
            if ( _left == 0 ) return 0;
            node *ret = _left;
            _left->remove();
            _count--;
            if ( _count == 0 ) _right = 0;
            return ret;
        }
        node *pop_back() {
            if ( _right == 0 ) return 0;
            node *ret = _right;
            _right->remove();
            _count--;
            _right = _count ? ret->_pre: 0;
            return ret;
        }
        void erase(node *pnode) {
            assert(pnode && size());
            pnode->remove();
            _count--;
        }
        size_t size() const { return _count; }
    };
}