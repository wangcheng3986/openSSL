#pragma once
namespace container_l0 {
    template <class T>
    class rbtree {
    public:
        typedef T KeyElement;
        class node {
        protected:
            friend class rbtree;
            node *_parent;
            node *_lchild;
            node *_rchild;
            char _color;
            KeyElement &key_ref;
        public:
            node(KeyElement &key):key_ref(key){}
            node *left() {

                node *ret = _lchild;
                if ( ret ) {
                    while ( ret->_rchild ) ret = ret->_rchild;
                    return ret;
                }
                node *top = this;
                for ( ret = _parent; ret; top = ret, ret = top->_parent ) {
                    if ( ret->_rchild == top ) return ret;
                }
                return 0;

            }
            node *next() {

                node *ret = 0;
                node *top_node = this;
                if ( top_node->_rchild ) {
                    for ( ret = top_node->_rchild; ret->_lchild; ret = ret->_lchild);
                    return ret;
                }

                for ( ret = top_node->_parent; ret; top_node = ret, ret = top_node->_parent ) {
                    if ( ret->_lchild == top_node ) return ret;
                }
                return 0;
            }
            const T &key() const { return key_ref;}
        };
    private:
        enum rbcolor {
            rbt_black = 0,
            rbt_red = 1
        };
        int _count;
        node *_root;
    public:
        int size() const { return _count; }
        rbtree(): _count(0), _root(0){}
        node *first() const {
            if ( _count == 0 ) return 0;
            node *ret = _root;
            while ( ret->_lchild ) ret = ret->_lchild;
            return ret;
        }
        node *last() const {
            if ( _count == 0 ) return 0;
            node *ret = _root;
            while ( ret->_rchild ) ret = ret->_rchild;
            return ret;
        }
        node *find(const T &_value) const {
            node *retval;
            node *tmp_node;

            if ( _count == 0 ) return 0;

            retval = _root;
            for (;;) {

                for (; retval && (_value < retval->key()); retval = retval->_lchild);
                if ( retval == 0 ) return 0;
                tmp_node = retval;

                for (;retval && (retval->key() < _value); retval = retval->_rchild);
                if ( retval == 0 ) return 0;

                if ( tmp_node == retval ) return retval;
            }

        };
        bool erase(node *obj) {
            if ( find(obj->key()) != obj ) return false;
            remove(obj);
            return true;
        }
        void remove(node *obj) {
            node *p;
            node *n;

            if ( _count == 0 ) return;

            if ( obj->_lchild && obj->_rchild ) {
                node *prev = obj->left();
                remove(prev);
                prev->_color = obj->_color;
                prev->_lchild = obj->_lchild; 
                if (obj->_lchild) obj->_lchild->_parent = prev;
                prev->_rchild = obj->_rchild; 
                if ( obj->_rchild) obj->_rchild->_parent = prev;
                p = prev->_parent = obj->_parent;
                if ( p ) {
                    if ( p->_lchild == obj ) p->_lchild = prev;
                    else p->_rchild = prev;
                }
                else _root = prev;
                return;
            }
            _count--;
            if ( _count == 0 ) {
                _root = 0; return;
            }

            p = obj->_parent;
            n = obj->_lchild?obj->_lchild:obj->_rchild;
            if ( p ) {
                if ( p->_lchild == obj ) p->_lchild = n;
                else p->_rchild = n;
            }
            if ( n ) n->_parent = p;
            if ( obj->_color == rbt_red ) return;

            for (;;) {
                node *u;
                node *uls;
                node *g;

                if ( p == 0 ) {
                    _root = n;// n->top = 0;
                    n->_color = rbt_black;
                    return;
                }

                if ( n && (n->_color == rbt_red) ) {
                    n->_color = rbt_black; return;
                }

                u = (p->_lchild == n)?p->_rchild: p->_lchild;

                if ( p->_color == rbt_black ) {
                    if ( u->_color == rbt_black ) {

                        if ( ((u->_lchild == 0) || (u->_lchild->_color == rbt_black)) &&
                            ((u->_rchild == 0) || (u->_rchild->_color == rbt_black)) ) {
                                u->_color = rbt_red;
                                n = p; p = n->_parent;
                                continue;
                        }
                        //u的两子至少有一个为红,后面处理
                    }
                    else {
                        g = p->_parent;
                        u->_parent = g;
                        if ( g ) {
                            if ( g->_lchild == p ) g->_lchild = u;
                            else g->_rchild = u;
                        }
                        else _root = u;

                        u->_color = rbt_black;
                        p->_color = rbt_red;
                        p->_parent = u;
                        if ( p->_rchild == u ) {
                            p->_rchild = u->_lchild; 
                            if ( u->_lchild ) u->_lchild->_parent = p;
                            u->_lchild = p;
                            u = p->_rchild;
                        }
                        else {
                            p->_lchild = u->_rchild; 
                            if ( u->_rchild ) u->_rchild->_parent = p;
                            u->_rchild = p;
                            u = p->_lchild;
                        }
                    }
                }
                if ( p->_color == rbt_red ) {
                    if ( ((u->_lchild == 0) || (u->_lchild->_color == rbt_black)) &&
                        ((u->_rchild == 0) || (u->_rchild->_color == rbt_black)) ) {
                            u->_color = rbt_red;
                            p->_color = rbt_black;
                            return;
                    }
                    //u的两子至少有一个为红,后面处理
                }
                //u至少有一子为红
                //u的同侧儿子为黑,旋转
                g = p->_parent;

                if ( p->_rchild == u ) {
                    if ( (u->_rchild == 0) || ( u->_rchild->_color == rbt_black ) ) {
                        uls = u->_lchild; 
                        u->_parent = uls; u->_lchild = uls->_rchild; 
                        if ( uls->_rchild ) uls->_rchild->_parent = u;
                        u->_color = rbt_red;
                        uls->_parent = p; uls->_rchild = u; uls->_color = rbt_black;
                        p->_rchild = uls;
                        u = uls;
                    }
                    p->_rchild = u->_lchild; 
                    if ( u->_lchild ) u->_lchild->_parent = p;
                    u->_rchild->_color = rbt_black;
                    u->_lchild = p;
                }
                else {
                    if ( (u->_lchild == 0) || ( u->_lchild->_color == rbt_black ) ) {
                        uls = u->_rchild; 
                        u->_parent = uls; u->_rchild = uls->_lchild; 
                        if ( uls->_lchild ) uls->_lchild->_parent = u;
                        u->_color = rbt_red;
                        uls->_parent = p; uls->_lchild = u; uls->_color = rbt_black;
                        p->_lchild = uls;
                        u = uls;
                    }
                    p->_lchild = u->_rchild; 
                    if ( u->_rchild ) u->_rchild->_parent = p;
                    u->_lchild->_color = rbt_black;
                    u->_rchild = p;
                }
                p->_parent = u;
                u->_parent = g; 
                u->_color = p->_color;
                p->_color = rbt_black;
                if ( g ) {
                    if ( g->_lchild == p ) g->_lchild = u;
                    else g->_rchild = u;
                }
                else _root = u;
                return;
            }
        }

        bool insert(node *obj) {
            const T &_value = obj->key();
            node *find_val = _root;
            bool done = false;
            node *p;//父节点
            node *g;//祖父节点
            node *u;//叔父节点
            node *gp;//曾祖父节点

            if ( _count == 0 ) {
                obj->_color = rbt_black;
                obj->_lchild = obj->_rchild = obj->_parent = 0;
                _root = obj;
                _count++;
                return true;
            }

            while ( ! done ) {
                node *tmp_node;

                for (; _value < find_val->key() ; find_val = find_val->_lchild) {

                    if ( find_val->_lchild == 0 ) {
                        obj->_color = rbt_red;
                        obj->_lchild = obj->_rchild = 0;
                        obj->_parent = find_val;
                        find_val->_lchild = obj;
                        _count++;
                        done = true; break; 
                    }
                }
                if ( done ) break;
                tmp_node = find_val;

                for (; find_val->key() < _value; find_val = find_val->_rchild) {

                    if ( find_val->_rchild == 0 ) { 
                        obj->_color = rbt_red;
                        obj->_lchild = obj->_rchild = 0;
                        obj->_parent = find_val;
                        find_val->_rchild = obj;
                        _count++;
                        done = true; break; 
                    }
                }
                if ( done ) break;
                if ( tmp_node == find_val ) return false;
            }

            for (;;) {

                if ( obj->_parent == 0 ) {
                    obj->_color = rbt_black; break;
                }
                p = obj->_parent;
                if ( p->_color == rbt_black ) break;
                g = p->_parent;
                u = (p == g->_lchild)?g->_rchild:g->_lchild;
                if ( (u == 0) || (u->_color == rbt_black) ) {
                    //同侧旋转
                    if ( g->_lchild == p ) {//左旋
                        if ( p->_rchild == obj ) {
                            g->_lchild = obj; 
                            obj->_parent = g;
                            p->_parent = obj; 
                            p->_rchild = obj->_lchild;
                            if ( obj->_lchild ) obj->_lchild->_parent = p;
                            obj->_lchild = p;

                            obj = p; p = g->_lchild;
                        }
                    }
                    else {//右旋
                        if ( p->_lchild == obj ) {
                            g->_rchild = obj; 
                            obj->_parent = g;
                            p->_parent = obj; 
                            p->_lchild = obj->_rchild;
                            if ( obj->_rchild ) obj->_rchild->_parent = p;
                            obj->_rchild = p;

                            obj = p; p = g->_rchild;
                        }
                    }

                    gp = g->_parent;

                    if ( g->_lchild == p ) {//右旋

                        g->_lchild = p->_rchild;
                        if ( p->_rchild ) p->_rchild->_parent = g;
                        p->_rchild = g; 
                    }
                    else {//左旋

                        g->_rchild = p->_lchild;
                        if ( p->_lchild ) p->_lchild->_parent = g;
                        p->_lchild = g;
                    }
                    g->_parent = p;
                    p->_parent = gp;
                    p->_color = rbt_black;
                    g->_color = rbt_red;

                    if ( gp ) {
                        if ( gp->_lchild == g ) gp->_lchild = p;
                        else gp->_rchild = p;
                    }
                    else _root = p;
                    break;
                }
                else {
                    u->_color = p->_color = rbt_black;
                    g->_color = rbt_red;
                    obj = g;
                }
            }

            return true;
        }

    };
}