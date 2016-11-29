#pragma once
#include "../common.h"
#include "../../L0/containers/rbtree.h"
#include <assert.h>
//#include "../../L2/allocator/wrapper_allocator.h"
namespace container_l2 {

    template <class Tx, class Ty>
    class tmp_node :public container_l0::rbtree<Tx>::node {
    public:
        virtual const Tx & key() const { return first; }
        typedef container_l0::rbtree<Tx> RB_Tree;
        Tx first;
        Ty second;
    public:
        tmp_node(const Tx &key, allocator_l0::allocator *Allocate)
            :RB_Tree::node(first){
                ::set_allocator(first, Allocate);
                first = key;
                ::set_allocator(second, Allocate);
        }

        tmp_node(const Tx &key, const Ty &value, allocator_l0::allocator *Allocate)
            :RB_Tree::node(first){
                ::set_allocator(first, Allocate);
                first = key;
                ::set_allocator(second, Allocate);
                second = value;
        }

        virtual ~tmp_node(){}
    };

    template <class T, class Y>
    class tmp_iterator {
    public:
        typedef container_l0::rbtree<T> TreeType;
        typedef typename TreeType::node * base_node_ptr;
        typedef tmp_node<T, Y> map_node;

    private:
        map_node *_node;
    public:
        tmp_iterator(map_node *node):_node(node){}
        tmp_iterator(const tmp_iterator &right) :_node(right._node){}
        tmp_iterator(base_node_ptr &treenode) :_node(static_cast<map_node*>(treenode)){}
        tmp_iterator &operator = (const tmp_iterator &right) {
            _node = right._node;
            return *this;
        }
        tmp_iterator &operator = (base_node_ptr rbtnode) {
            _node = static_cast<map_node*>(rbtnode);
            return *this;
        }
        tmp_iterator():_node(0){}
        tmp_iterator &operator ++ () {
            if ( _node ) _node = static_cast<map_node*>(_node->next());
            return *this;
        }
        tmp_iterator &operator -- () {
            if ( _node ) _node = static_cast<map_node*>(_node->left());
            return *this;
        }
        map_node &operator * () const {
            assert(_node);
            return *_node;
        }
        map_node * operator -> () const {
            assert(_node);
            return _node;
        }

        bool operator == (const tmp_iterator &right) {
            return _node == right._node;
        }
        bool operator != (const tmp_iterator &right) {
            return ! operator == (right);
        }
    };

    template <class T, class Y> 
    class map :public container_l0::rbtree<T>, public allocator_l0::contain_base {
    public:
        typedef container_l0::rbtree<T> RBTree;
        typedef typename RBTree::node * base_node_ptr;
        typedef tmp_node<T, Y> map_node;
        typedef tmp_iterator<T, Y> iterator;
    public:
        typedef map<T, Y> my_type;
        map():contain_base(0){}
        map(allocator_l0::allocator *Allocate):contain_base(Allocate){}
        map(const my_type &val):contain_base(val.get_allocator()){

            for ( iterator it = val.begin(); it != val.end(); ++it ) {
                insert(it->first, it->second);
            }
        }
        ~map() {
            clear();
        }
        void clear() {
            while (RBTree::size()) {
                map_node *the_node = static_cast<map_node*>(RBTree::first());
                RBTree::remove(the_node);
                allocator_l0::default_allocator tmp_alloc(_allocator);
                the_node->~map_node();
                tmp_alloc.deallocate(the_node);
            }
        }
        my_type &operator = (const my_type &val) {

            clear();
            if ( get_allocator() == 0 ) set_allocator(val.get_allocator());
            for ( iterator it = val.begin(); it != val.end(); ++it ) {
                insert(it->first, it->second);
            }
            return *this;
        }
        iterator insert(const T &key, const Y &value) {

            map_node *ep_node = 0;
            typename RBTree::node *node_ptr = 0;
            if ( (node_ptr = RBTree::find(key)) == 0 ) {
                allocator_l0::default_allocator tmp_alloc(_allocator);
                ep_node = static_cast<map_node*>(tmp_alloc.allocate(sizeof(map_node)));
                new ((void*)ep_node) map_node(key, value, _allocator);
                RBTree::insert(ep_node);
            }
            else ep_node = static_cast<map_node*>(node_ptr);
            return iterator(ep_node);
        }

        Y &operator [] (const T &key) {

            typename RBTree::node *node = 0;
            if ( (node = RBTree::find(key)) == 0 ) {
                allocator_l0::default_allocator tmp_alloc(_allocator);
                map_node *ep_node = static_cast<map_node*>(tmp_alloc.allocate(sizeof(map_node)));
                new ((void*)ep_node) map_node(key, _allocator);
                node = ep_node;
                RBTree::insert(node);
            }
            map_node *ep_node = static_cast<map_node*>(node);
            return ep_node->second;
        }
        bool erase(const T & key) {
            typename RBTree::node *treenode = RBTree::find(key);
            if ( treenode ) {
                RBTree::remove(treenode);
                map_node *mapnode = static_cast<map_node*>(treenode);
                allocator_l0::default_allocator tmp_alloc(_allocator);
                mapnode->~map_node();
                tmp_alloc.deallocate(mapnode);
            }
            return treenode != 0;
        }
        bool erase(const iterator it) {
            typename RBTree::node *treenode = it.operator -> ();
            if ( treenode ) {
                RBTree::remove(treenode);
                map_node *mapnode = static_cast<map_node*>(treenode);
                allocator_l0::default_allocator tmp_alloc(_allocator);
                mapnode->~map_node();
                tmp_alloc.deallocate(mapnode);
            }
            return treenode != 0;
        }

        iterator begin() const {
            base_node_ptr treenode = RBTree::first();
            if ( treenode ) return iterator(static_cast<map_node*>(treenode));
            return iterator();
        }
        iterator end() const { return iterator(); }
        iterator find(const T &value) const {
            base_node_ptr treenode = RBTree::find(value);
            if ( treenode ) return iterator(static_cast<map_node*>(treenode));
            return iterator();
        }
    };
}

