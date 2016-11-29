#pragma once
#include "../common.h"
#include <assert.h>
namespace container_l2 {

    template<class Elem>
    struct Elem_Traits {
        typedef Elem char_type;
        typedef long int_type;
        static void  assign(Elem& left, const Elem& right)
        {	
            left = right;
        }

        static bool  eq(const Elem& left, const Elem& right)
        {	
            return (left == right);
        }

        static bool  lt(const Elem& left, const Elem& right)
        {	
            return (left < right);
        }

        static int  compare(const Elem *left, const Elem *right, size_t count)
        {
            for (; 0 < count; --count, ++left, ++right) {
                if (!eq(*left, *right)) return (lt(*left, *right) ? -1 : +1);
            }
            return (0);
        }

        static size_t  length(const Elem *src)
        {
            size_t ret;
            for (ret = 0; !eq(*src, Elem()); ++src) {
                ++ret;
            }
            return (ret);
        }
        
        static Elem * copy(Elem *left, const Elem *right, size_t count)
        {
            return copy_string(left, count, right, count);
        }
        static void s_copy(Elem *left, const Elem *right, size_t count)
        {
            copy(left, right, count);
            assign(left[count], Elem());
        }

        static Elem * copy_string(Elem *left, size_t _Dest_size, const Elem *right, size_t count)
        {
            Elem *next = left;
            for (; 0 < count; --count, ++next, ++right) {
                assign(*next, *right);
            }
            return (left);
        }

        static const Elem * find(const Elem *src, size_t count, const Elem& ch)
        {
            for ( ; 0 < count; --count, ++src ) {
                if ( eq(*src, ch) ) return (src);
            }
            return (0);
        }
        
        static Elem * move(Elem *left, const Elem *right, size_t count)
        {
            return move_string(left, count, right, count);
        }

        static Elem * move_string(Elem *left, size_t _Dest_size, const Elem *right, size_t count)
        {
            assert(_Dest_size >= count);
            Elem *next = left;
            if (right < next && next < right + count) {
                for (next += count, right += count; 0 < count; --count) {
                    assign(*--next, *--right);
                }
            }
            else {
                for (; 0 < count; --count, ++next, ++right) {
                    assign(*next, *right);
                }
            }
            return (left);
        }

        static Elem * assign(Elem *src, size_t count, Elem ch)
        {
            Elem *next = src;
            for (; 0 < count; --count, ++next) {
                assign(*next, ch);
            }
            return (src);
        }

        static Elem to_char_type(const int_type& raw_val)
        {
            return ((Elem)raw_val);
        }

        static int_type  to_int_type(const Elem& ch)
        {
            return ((int_type)ch);
        }

        static bool  eq_int_type(const int_type& left, const int_type& right)
        {
            return (left == right);
        }

        static int_type  eof()
        {
            return ((int_type)EOF);
        }

        static int_type  not_eof(const int_type& raw_val)
        {
            return (raw_val != eof() ? (int_type)raw_val : (int_type)!eof());
        }
    };


    template <class Elem, int raw_size = 4>
    class base_string :public allocator_l0::contain_base {
    public:

        typedef base_string<Elem, raw_size> my_type;

        typedef Elem_Traits<Elem> my_traits;

        typedef allocator_l0::my_allocator<Elem> my_allocator;
        typedef typename my_allocator::size_type size_type;
        typedef typename my_allocator::difference_type _Dift;
        typedef _Dift difference_type;
        typedef typename my_allocator::pointer _Tptr;
        typedef typename my_allocator::const_pointer _Ctptr;
        typedef _Tptr pointer;
        typedef _Ctptr const_pointer;
        typedef typename my_allocator::reference _Reft;
        typedef _Reft reference;
        typedef typename my_allocator::const_reference const_reference;
        typedef typename my_allocator::value_type value_type;

        base_string():_size(0){_data._buf[0] = Elem();}
        base_string(const_pointer src):_size(0) { assign(src); }
        base_string(const_pointer src, size_type len) :_size(0){ assign(src, len); }
        base_string(const_pointer src, allocator_l0::allocator * alloc) :contain_base(alloc), _size(0){ assign(src); }
        base_string(allocator_l0::allocator * alloc) :contain_base(alloc), _size(0){_data._buf[0] = Elem();}
        base_string(allocator_l0::allocator * alloc, const_pointer src, size_type len) :contain_base(alloc), _size(0){
            assign(src, len);
        }
        base_string(const_pointer src, size_type len, allocator_l0::allocator * alloc) :contain_base(alloc), _size(0){
            assign(src, len);
        }
        base_string(const my_type &val) :contain_base(val.get_allocator()), _size(0) { assign(val); }
        my_type &assign(const Elem *src) {  return assign( src, my_traits::length(src) );  }
        my_type &assign(const my_type &val) {  return assign( val.c_str(), val.size() );  }
        my_type & operator = (const Elem *src) { return assign(src); }
        my_type & operator = (const my_type &val) { return assign(val); }
        my_type &assign(const Elem *src, size_type len) {
            if ( len == _size ) {
                my_traits::s_copy(elem_buf(), src, len);
            }
            else {
                _release();
                Elem *buffer;
                if ( len >= raw_size ) {
                    allocator_l0::default_allocator tmp_allocate(_allocator);
                    _data._data = static_cast<Elem*>(tmp_allocate.allocate((len + 1) * sizeof(Elem)));
                    buffer = _data._data;
                }
                else buffer = _data._buf;
                my_traits::s_copy(buffer, src, len);
                _size = len;
            }
            return *this;
        }
        my_type &append(const my_type &src) { return append(src.c_str(), src.size()); }
        my_type &append(const Elem *src) {  return append(src, my_traits::length(src));  }
        my_type &append(const Elem *src, size_type len) {

            if ( _size + len < raw_size ) {
                my_traits::s_copy(_data._buf + _size, src, len);
                _size += len;
            }
            else {
                allocator_l0::default_allocator tmp_allocate(_allocator);
                Elem *buffer = static_cast<Elem*>(tmp_allocate.allocate((_size + len + 1) * sizeof(Elem)) );
                my_traits::copy(buffer, elem_buf(), _size);
                my_traits::s_copy(buffer + _size, src, len);
                size_type new_size = _size + len;
                _release();
                _data._data = buffer;
                _size = new_size;
            }
            return *this;
        }
        bool operator < ( const my_type &right ) const {
            return compare(0, _size, right.elem_buf(), right._size) < 0;
        }
        bool operator == ( const my_type &right ) const {
            return compare(0, _size, right.elem_buf(), right._size) == 0;
        }
        bool operator == ( const Elem *src ) const {
            return compare(0, _size, src, my_traits::length(src)) == 0;
        }
        bool operator != ( const Elem *src ) const {
            return ! (operator == (src));
        }
        size_type size() const { return _size; }
        int compare(size_type offset, size_type num, const Elem *src, size_type count) const
        {	
            assert(offset <= _size);
            if (_size - offset < num) num = _size - offset;
            size_type ret = my_traits::compare(elem_buf() + offset, src, num < count ? num: count);
            return (ret ? (int)ret : num < count ? -1 : num == count ? 0 : +1);
        }

        const Elem *c_str() const { return elem_buf(); }

        void clear() { _release(); }
        bool empty() const { return _size == 0; }
        size_type length() const { return _size; }
        ~base_string() {
            if ( _size >= raw_size ) {
                allocator_l0::default_allocator tmp_allocate(_allocator);
                tmp_allocate.deallocate(_data._data);
            }
        }
    private:
        void _release() {
            if ( _size >= raw_size ) {
                allocator_l0::default_allocator tmp_allocate(_allocator);
                tmp_allocate.deallocate(_data._data);
            }
            _size = 0;
        }
        Elem *elem_buf() const {  return const_cast<Elem*>(( _size < raw_size ) ? _data._buf: _data._data);  }
        union MyContain {
            Elem *_data;
            Elem _buf[raw_size];
        };
        size_type _size;
        MyContain _data;
    };
    typedef base_string<char, 16> string;
}
