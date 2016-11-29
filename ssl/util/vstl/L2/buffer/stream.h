#pragma once
#include "../common.h"
#include "../../L0/abstract/unistd.h"
#include "../../L0/containers/list.h"
#include <string.h>
#include <memory.h>
namespace container_l2 {
    template <word_t node_size>
    class stream :public allocator_l0::contain_base {
        dword_t _used;
        container_l0::list _list;
    public:
        typedef stream<node_size> my_type;
        struct node :public container_l0::list::node {
        protected:
            word_t _used;
            word_t _start;
            char _data[node_size];
        public:
            node() { init(); }
            void init() {
                _used = _start = 0;
            }
            word_t append(const char *data, word_t len) { return push_back(data, len); }
            word_t push_back(const char *data, word_t len) {

                word_t ret = unistd_l0::Min<word_t>(len, node_size - _used);//copy size
                word_t begin = _start + _used;//copy begin
                begin -= (begin >= node_size) ? node_size: 0;
                write(begin, data, ret);
                _used += ret;
                return ret;
            }
            word_t push_front(const char *data, word_t len) {

                word_t ret = unistd_l0::Min<word_t>(len, node_size - _used);//copy size
                if ( ret < len ) data += len - ret;
                _start = _start + node_size - ret;
                _start -= (_start >= node_size) ? node_size: 0;
                write(_start, data, ret);
                _used += ret;
                return ret;
            }
            word_t pop_front( char *buf, word_t len ) {
                word_t ret = unistd_l0::Min(len, _used);//copy size
                if ( buf ) read(_start, buf, ret);
                _start += ret;
                _start -= (_start >= node_size) ? node_size: 0;
                _used -= ret;
                return ret;
            }
            word_t pop_back( char *buf, word_t len ) {
                word_t ret = unistd_l0::Min(len, _used);//copy size
                word_t begin = _start + _used - ret;
                begin -= (begin >= node_size) ? node_size: 0;
                if ( buf ) read(begin, buf, ret);
                _used -= ret;
                return ret;
            }
            word_t replace(const char *data, int len, word_t offset) {
                if ( _used <= offset ) return 0;
                word_t ret = unistd_l0::Min(len, _used - offset);//copy size
                word_t begin = _start + offset;
                begin -= (begin >= node_size) ? node_size: 0;
                write(begin, data, ret);
                return ret;
            }
            word_t peek(char *buf, int len, word_t offset) const {
                if ( _used <= offset ) return 0;
                word_t ret = unistd_l0::Min(len, _used - offset);//copy size
                word_t begin = _start + offset;
                begin -= (begin >= node_size) ? node_size: 0;
                read(begin, buf, ret);
                return ret;
            }
            bool empty() { return _used == node_size; }
            word_t size() const { return _used; } 
            char &offset(word_t position) {

                word_t off = _start + position;
                off -= (off >= node_size) ? node_size: 0;
                return _data[off];
            }
        private:
            void write(word_t begin, const char *data, int len) {

                word_t copy_first = unistd_l0::Min(len, node_size - begin);
                memcpy(_data + begin, data, copy_first);
                if ( copy_first < len ) memcpy(_data, data + copy_first, len - copy_first);
            }
            void read(word_t begin, char *data, int len) const {

                word_t copy_first = unistd_l0::Min(len, node_size - begin);
                memcpy(data, _data + begin, copy_first);
                if ( copy_first < len ) memcpy(data + copy_first, _data, len - copy_first);
            }
        };
        class iterator {
        protected:
            node *_node;
            word_t _offset;
            friend class stream;
        public:
            iterator():_node(0), _offset(0){}
            iterator &operator ++ () {
                if ( _node ) {
                    _offset++;
                    if ( _node->size() <= _offset ) {
                        _node = static_cast<node *>(_node->next());
                        _offset = 0;
                    }
                }
                return *this;
            }
            iterator & operator += (dword_t offset) {
                
                while ( _node && offset ) {

                    word_t move_off = (word_t)unistd_l0::Min<dword_t>(offset, _node->size() - _offset);
                    _offset += move_off;
                    offset -= move_off;
                    if ( _offset < _node->size() ) break;
                    _node = static_cast<node *>(_node->next());
                    _offset = 0;
                }
                return *this;
            }
            iterator & operator -= (dword_t offset) {

                while ( _node ) {

                    word_t move_off = (word_t)unistd_l0::Min<dword_t>(offset, _offset);
                    _offset -= move_off;
                    offset -= move_off;
                    if ( offset == 0 ) break;
                    _node = static_cast<node *>(_node->pre());
                    _offset = _node ? _node->size(): 0;
                }
                return *this;
            }
            iterator & operator -- () {
                return operator -= (1);
            }

            char &operator * () {
                assert(_node);
                return _node->offset(_offset);
            }
            bool operator == (const iterator &right) {
                return _node == right._node && _offset == right._offset;
            }
            bool operator != (const iterator &right) {
                return ! operator == (right);
            }
        };
        iterator begin() const {
            iterator ret;
            word_t node_off = 0;
            ret._node = seek(0, ret._offset);
            return ret;
        }
        iterator rbegin() const {
            iterator ret;
            node *top_node = static_cast<node *>(_list.back());
            if ( top_node ) {
                ret._node = top_node;
                ret._offset = top_node->size() ? top_node->size() - 1: 0;
            }
            return ret;
        }
        iterator end() const {
            return iterator();
        }
        stream():_used(0){}
        stream(allocator_l0::allocator *alloc_inst):allocator_l0::contain_base(alloc_inst), _used(0){}
        void clear() {
            while ( _list.size() ) {
                node *top_node = static_cast<node *>(_list.pop_front());
                allocator_l0::default_allocator tmp_alloc(_allocator);
                tmp_alloc.deallocate(top_node);
            }
            _used = 0;
        }
        ~stream() { clear(); }

        dword_t size() const { return _used; }
        stream(const my_type &val) throw()
            :_used(0), allocator_l0::contain_base(val.get_allocator()) {
            append(val);
        }

        template<word_t _Other>
        stream(const stream<_Other> &val) throw ()
            :_used(0), allocator_l0::contain_base( val.get_allocator()) {
            append(val);
        }

        

        template<word_t _Other>
        stream<node_size>& operator=(const stream<_Other>& val) throw()
        {
            _allocator = _allocator ?_allocator: val.get_allocator();
            clear();
            append(val);
            return (*this);
        }

        template<word_t _Other>
        dword_t append(const stream<_Other> &source) throw() {

	    typedef stream<_Other> _OtherType;
            if ( source.size() == 0 ) return 0;
            dword_t ret = 0;
            char temp_buf[_Other];
            word_t offset_node = 0;
            for ( const typename _OtherType::node *other_node = static_cast<const typename _OtherType::node *>(source.seek(0, offset_node));
                other_node; 
                other_node = static_cast<const typename stream<_Other>::node *>(other_node->next())
                ) {

                dword_t size = other_node->peek(temp_buf, sizeof(temp_buf), 0);
                dword_t pushed = push_back(temp_buf, size);
                ret += pushed;
                if ( pushed != size ) break;
            }
            return ret;
        }

        iterator find(const char *data, dword_t len) const {
            iterator it = begin();
            if ( size() < len ) return end();
            for ( dword_t i = 0; i <= size() - len; ++i, ++it ) {

                iterator tmp = it;
                dword_t j = 0;
                for ( ; (j < len) && (*tmp == data[j]); ++j, ++tmp );
                if ( j == len ) return it;
            }
            return iterator();
        }
        iterator find(const char *data, dword_t len, int &offset) const {
            iterator it = begin();
            offset = -1;
            if ( size() < len ) return end();
            dword_t i = 0;
            for ( ; i <= size() - len; ++i, ++it ) {

                iterator tmp = it;
                dword_t j = 0;
                for ( ; (j < len) && (*tmp == data[j]); ++j, ++tmp );
                if ( j == len ) {
                    offset = (int)i;
                    return it;
                }
            }
            return iterator();
        }
        dword_t push_back(const char *data, dword_t len) {
            dword_t ret = 0;
            while ( len ) {

                node *top_node = static_cast<node *>(_list.back());
                if ( top_node == 0 || top_node->empty() ) {
                    allocator_l0::default_allocator tmp_alloc(_allocator);
                    top_node = (node *)tmp_alloc.allocate(sizeof(node));
                    if ( top_node == 0 ) break;;
                    top_node->init();
                    _list.push_back(top_node);
                }
                word_t push_max = (word_t)unistd_l0::Min((dword_t)node_size, len);
                word_t writed = top_node->push_back(data + ret, push_max);
                _used += writed;
                ret += writed;
                len -= writed;
            }
            return ret;
        }
        dword_t push_front(const char *data, dword_t len) {
            dword_t ret = 0;
            while ( len ) {

                node *top_node = static_cast<node *>(_list.front());
                if ( top_node == 0 || top_node->empty() ) {
                    allocator_l0::default_allocator tmp_alloc(_allocator);
                    top_node = (node*)tmp_alloc.allocate(sizeof(node));
                    if ( top_node == 0 ) break;
                    top_node->init();
                    _list.push_front(top_node);
                }
                word_t push_max = (word_t)unistd_l0::Min((dword_t)node_size, len);
                word_t writed = top_node->push_front(data + len - push_max, push_max);
                _used += writed;
                ret += writed;
                len -= writed;
            }
            return ret;
        }
        dword_t pop_front(dword_t len, char *buf = 0) {
            len = (dword_t)unistd_l0::Min<dword_t> (_used, len);
            dword_t ret = len;
            while ( len ) {
                node *top_node = static_cast<node *>(_list.front());
                word_t poped = top_node->pop_front(buf, (word_t)unistd_l0::Min((dword_t)node_size, len));
                len -= poped; 
                buf += buf ? poped : 0;
                if ( top_node->size() == 0 ) {
                    _list.pop_front();
                    allocator_l0::default_allocator tmp_alloc(_allocator);
                    tmp_alloc.deallocate(top_node);
                }
            }
            _used -= ret;
            return ret;
        }
        dword_t pop_back(dword_t len, char *buf = 0) {
            len = (dword_t)unistd_l0::Min (_used, len);
            dword_t ret = len;
            while ( len ) {
                node *top_node = static_cast<node *>(_list.back());
                word_t poped = (word_t)unistd_l0::Min (len, (dword_t)top_node->size());
                top_node->pop_back(buf ?buf + len - poped: 0, poped);
                len -= poped;
                if ( top_node->size() == 0 ) {
                    _list.pop_back();
                    allocator_l0::default_allocator tmp_alloc(_allocator);
                    tmp_alloc.deallocate(top_node);
                }
            }
            _used -= ret;
            return ret;
        }
        dword_t peek(char *buf, dword_t len, dword_t offset = 0) const {
            word_t node_offset = 0;
            node *top_node = seek(offset, node_offset);
            if ( top_node == 0 ) return 0;

            dword_t ret = (dword_t)unistd_l0::Min (_used - offset, len);
            dword_t peeked = 0;
            while ( peeked < ret ) {
                word_t node_peeked = top_node->peek(buf + peeked, (word_t)unistd_l0::Min<dword_t>(ret - peeked, top_node->size() - node_offset), node_offset);
                peeked += node_peeked;
                node_offset = 0;
                top_node = static_cast<node *>(top_node->next());
            }
            return ret;
        }
        dword_t replace(dword_t offset, const char *data, dword_t len) {

            word_t node_offset = 0;
            node *top_node = seek(offset, node_offset);
            if ( top_node == 0 ) return 0;

            dword_t ret = (dword_t)unistd_l0::Min<dword_t> (_used - offset, len);
            dword_t writed = 0;
            while ( writed < ret ) {
                word_t node_writed = top_node->replace(data + writed, (word_t)unistd_l0::Min<dword_t>(ret - writed, top_node->size() - node_offset), node_offset);
                writed += node_writed;
                node_offset = 0;
                top_node = static_cast<node *>(top_node->next());
            }
            return ret;
        }

        node *seek(dword_t offset, word_t &node_offset) const {

            if ( _used <= offset ) return 0;
            dword_t seeked = 0;
            node *top_node = static_cast<node *>(_list.front());
            while ( seeked + top_node->size() <= offset ) {
                seeked += top_node->size();
                top_node = static_cast<node *>(top_node->next());
            }
            node_offset = (word_t)(offset - seeked);
            return top_node;
        }
        int offset(iterator it) {
            if ( it == end() ) return -1;
            int ret = 0;
            node *top_node = static_cast<node *>(_list.front());
            while ( top_node ) {
                if ( top_node == it._node ) {
                    assert(it._offset < top_node->size());
                    return ret + it._offset;
                }
                ret += top_node->size();
                top_node = static_cast<node *>(top_node->next());
            }
            return -1;
        }
    };
    class stream_wrapper {
    public:
        virtual dword_t push_back(const char *data, dword_t len) = 0;
        virtual dword_t push_front(const char *data, dword_t len) = 0;
        virtual dword_t pop_front(dword_t len, char *buf = 0) = 0;
        virtual dword_t pop_back(dword_t len, char *buf = 0) = 0;
        virtual dword_t peek(char *buf, dword_t len, dword_t offset = 0) const = 0;
        virtual dword_t replace(dword_t offset, const char *data, int len) = 0;
        virtual int find(const char *data, int len) const = 0;
        virtual dword_t size() = 0;

    };
    template <word_t node_size>
    class my_stream_wrapper :public stream_wrapper {
        typedef stream<node_size> stream_type;
        stream_type &_stream;
    public:
        my_stream_wrapper(stream<node_size> &stream_ref) :_stream(stream_ref) {}
        my_stream_wrapper(my_stream_wrapper &right) :_stream(right._stream) {}

        virtual dword_t push_back(const char *data, dword_t len) { return _stream.push_back(data, len);}
        virtual dword_t push_front(const char *data, dword_t len) { return _stream.push_front(data, len);}
        virtual dword_t pop_front(dword_t len, char *buf = 0) { return _stream.pop_front(len, buf);}
        virtual dword_t pop_back(dword_t len, char *buf = 0) { return _stream.pop_back(len, buf);}
        virtual dword_t peek(char *buf, dword_t len, dword_t offset = 0) const { return _stream.peek(buf, len, offset);}
        virtual dword_t replace(dword_t offset, const char *data, int len) { return _stream.replace(offset, data, len);}
        virtual int find(const char *data, int len) const {
            int ret = -1;
            typename stream_type::iterator it = _stream.find(data, len, ret);
            return ret;
//             for ( ; ret <= (int)_stream.size() - len; ++ret, ++it ) {
// 
//                 typename stream_type::iterator tmp = it;
//                 int j = 0;
//                 for ( ; (j < len) && (*tmp == data[j]); ++j, ++tmp );
//                 if ( j == len ) return ret;
//             }
//             return -1;
        }
        virtual dword_t size() { return _stream.size();}

    };
    template <word_t node_size>
    my_stream_wrapper<node_size> get_wrapper(stream<node_size> &stream_obj) {
        return my_stream_wrapper<node_size>(stream_obj);
    }
};

