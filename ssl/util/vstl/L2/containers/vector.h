#pragma once
#include "../common.h"
#include "../../L0/abstract/unistd.h"
namespace container_l2 {
    using namespace type_l0;

    template < bool fast = true >
    struct Segment {
        byte_t _size;
        byte_t _use;
        byte_t _begin;
        byte_t _end;
        Segment<fast> *_sub[1];
        void use() { _use++; }
        void release() { _use--; }
        byte_t used() const { return _use;}
        static size_t need_size(byte_t count) {

            return sizeof( Segment<fast> ) + sizeof(Segment<fast> *) * count;
        }
        Segment(const Segment &old, byte_t offset)
            : _size(offset)
            , _use(old._use + 1)
            , _begin(old._begin)
            , _end(offset)
        {
            assert(old._size < offset);
            for ( word_t i = 0; i <= old._size; i++ ) 
                _sub[i] = old._sub[i];

            for ( word_t i = old._size + 1; i <= _size; i++ )
                _sub[i] = 0;
        }
        Segment(byte_t offset) :_size(fast? 255: offset), _use(0), _begin(offset), _end(offset) {

            for ( word_t i = 0; i <= (fast ?255: (word_t)offset); i++ ) 
                _sub[i] = 0;
        }
        ~Segment(){}

        Segment<fast> * find_next_id(byte_t &id) const {
            if ( id >= (word_t)_end ) return 0;
            for ( id++; _sub[id] == 0; id++ );
            return _sub[id];
        }
        Segment<fast> * find_pre_id(byte_t &id) const {
            if ( id <= _begin ) return 0;
            for ( id--; _sub[id] == 0; id-- );
            return _sub[id];
        }
    };

    struct SegHelper {
        dword_t &_size;
        allocator_l0::allocator *_alloc;
        SegHelper(allocator_l0::allocator *alloc, dword_t &size):_alloc(alloc), _size(size){}
    };
    template <byte_t level, byte_t range>
    class Endian {
    public:
        static byte_t index() {
            dword_t tmp = 1;
            bool littleEndian = 1 == *(byte_t *)&tmp;
            return littleEndian ? level - 1   : range - (level - 1);
        }
    };
    template <class T, byte_t level, bool fast>
    class loader {
    public:

        static void first(Segment<fast> *This, dword_t &id) {

            byte_t offset = This->_begin;
            ((char*)&id)[Endian<level, 4>::index()] = offset;
            loader<T, level - 1, fast>::first(This->_sub[offset], id);
        }

        static void last(Segment<fast> *This, dword_t &id) {

            byte_t offset = This->_end;
            ((char*)&id)[Endian<level, 4>::index()] = offset;
            loader<T, level - 1, fast>::last(This->_sub[offset], id);
        }

        static bool next(Segment<fast> *This, dword_t &id) {
            byte_t offset = (id >> ( ( level - 1 ) * 8 )) & 0xFF;
            if ( ! loader<T, level - 1, fast>::next(This->_sub[offset], id) ) {
                if ( ! This->find_next_id(offset) ) return false;
                 dword_t bit_mask = ~( 0xFF << ( (level - 1) * 8) );
                 id = ( id & bit_mask ) | ( (dword_t)offset  << ( ( level - 1) * 8 ) ); 

                loader<T, level - 1, fast>::first(This->_sub[offset], id);
            }
            return true;
        }
        static bool pre(Segment<fast> *This, dword_t &id) {
            byte_t offset = (id >> ( ( level - 1 ) * 8 )) & 0xFF;
            if ( ! loader<T, level - 1, fast>::pre(This->_sub[offset], id) ) {
                if ( ! This->find_pre_id(offset) ) return false;
                dword_t bit_mask = ~( 0xFF << ( (level - 1) * 8) );
                id = ( id & bit_mask ) | ( (dword_t)offset  << ( ( level - 1) * 8 ) ); 
                loader<T, level - 1, fast>::last(This->_sub[offset], id);
            }
            return true;
        }

        static T &load(Segment<fast> * &seg, dword_t index, SegHelper &wrapper) {
            byte_t offset = ( index >> ( (level - 1) * 8 ) ) & 0xFF;

            if ( (seg == 0) || (fast? false: seg->_size < offset) ) {

                Segment<fast> *data = (Segment<fast> *)wrapper._alloc->allocate( Segment<fast>::need_size(fast?255:offset) );
                if ( fast? 0: seg ) {
                    new ((void *) data) Segment<fast>(*seg, offset);
                    seg->Segment<fast>::~Segment();
                    wrapper._alloc->deallocate(seg);
                }
                else new ((void*)data) Segment<fast>(offset);
                seg = data;
            }
            else {
                if ( seg->_sub[offset] == 0 ) seg->use();
                assert(seg->used() <= seg->_size);
            }
            if (offset < seg->_begin ) seg->_begin = offset;
            else if ( offset > seg->_end ) seg->_end = offset;
            return loader<T, level - 1, fast>::load(seg->_sub[offset], index, wrapper);
        }

        static bool destroy(Segment<fast> * &seg, dword_t index, SegHelper &wrapper) {

            byte_t offset = ( index >> ( (level - 1) * 8 ) ) & 0xFF;
            if( ! fast ) {
                if ( seg->_size < offset ) return false;
            }
            if ( seg->_sub[offset] == 0 ) return false;

            bool ret = loader<T, level - 1, fast>::destroy(seg->_sub[offset], index, wrapper);//exist
            if ( seg->_sub[offset] == 0 ) {
                seg->release();
                if ( seg->used() == 255 ) {
                    seg->Segment<fast>::~Segment();
                    wrapper._alloc->deallocate(seg);
                    seg = 0;
                }
                else {
                    if ( offset == seg->_end ) {
                        while ( seg->_sub[seg->_end] == 0 ) seg->_end--;
                    }
                    else if ( offset == seg->_begin ) {
                        while ( seg->_sub[seg->_begin] == 0 ) seg->_begin++;
                    }
                }
            }
            return ret;
        }
        static bool exist(Segment<fast> * &seg, dword_t index, SegHelper &wrapper) {
            if ( seg == 0 ) return false;
            byte_t offset = ( index >> ( (level - 1) * 8 ) ) & 0xFF;
            if( ! fast ) {
                if ( seg->_size < offset ) return false;
            }
            return loader<T, level - 1, fast>::exist(seg->_sub[offset], index, wrapper);//exist
        }
    };
    template <class T, bool fast>
    class loader<T, 0, fast> {
    public:
        static void first(Segment<fast> *This, dword_t &id) {}
        static void last(Segment<fast> *This, dword_t &id) {}
        static bool next(Segment<fast> *This, dword_t &id) { return false; }
        static bool pre(Segment<fast> *This, dword_t &id) { return false; }

        static T & load(Segment<fast> * &seg, dword_t index, SegHelper &wrapper) {
            if ( seg == 0 ) {

                T *value = (T*)wrapper._alloc->allocate(sizeof(T));
                new ((void*)value) T();
                ::set_allocator(*value, wrapper._alloc);
                seg = (Segment<fast> *)value;
                wrapper._size++;
            }
            return *(T*)seg;
        }
        static bool destroy(Segment<fast> * &seg, dword_t index, SegHelper &wrapper) {

            T *val = (T*)(seg);
            val->~T();
            wrapper._alloc->deallocate(val);
            wrapper._size--;
            seg = 0;
            return true;
        }
        static bool exist(Segment<fast> * &seg, dword_t index, SegHelper &wrapper) {
            return seg != 0;
        }
    };

    template <typename T, bool fast=false> 
    class vector :public allocator_l0::contain_base{

        dword_t _size;
    protected:
        Segment<fast> *_data;
    public:
        typedef vector<T, fast> my_type;
        class iterator {
            vector *_vec;
        public:
            dword_t _index;
            bool _valid;
            iterator(vector *vec):_vec(vec), _valid(false), _index(0){}
            iterator(const iterator &right) :_vec(right._vec), _valid(right._valid), _index(right._index){}
            iterator &operator = (const iterator &right) {
                _vec = right._vec;
                _valid = right._valid;
                _index = right._index;
            }
            iterator():_vec(0), _valid(false), _index(0){}
            iterator &operator ++ () {
                if ( _valid ) _index = _vec->next(_index, _valid);
                return *this;
            }
            iterator &operator -- () {
                if ( _valid ) _index = _vec->pre(_index, _valid);
                return *this;
            }
            T &operator * () {
                assert(_valid);
                return _vec->operator [](_index);
            }
            bool operator == (const iterator &right) {
                return _valid == right._valid && _index == right._index && _vec == right._vec;
            }
            bool operator != (const iterator &right) {
                return ! operator == (right);
            }
            dword_t offset() const { return _index; }
        };
    protected:
        friend class iterator;
        dword_t next(dword_t index, bool &valid) const {

            valid = _data ? loader<T, 4, fast>::next(_data, index): false;
            return valid ? index: 0;
        }
        dword_t pre(dword_t index, bool &valid) const {

            valid = _data ? loader<T, 4, fast>::pre(_data, index): false;
            return valid ? index: 0;
        }
    public:
        vector(allocator_l0::allocator *Alloc) :contain_base(Alloc), _data(0), _size(0){}
        vector(const vector &val):contain_base(val._allocator), _data(0),_size(0){
            for ( iterator it = val.begin(); it != val.end(); ++it ) {
                (*this)[it->offset()] = *it;
            }
        }
        ~vector() { clear(); }
        void clear() { while ( size() ) erase(begin());}
        iterator begin() const {

            if ( _size == 0 ) return end();
            iterator ret(const_cast<vector *>(this));
            loader<T, 4, fast>::first(_data, ret._index);
            ret._valid = true;
            return ret;
        }
        iterator rbegin() const {

            if ( _size == 0 ) return end();
            iterator ret(const_cast<vector *>(this));
            ret._valid = true;
            loader<T, 4, fast>::last(_data, ret._index);
            return ret;
        }
        iterator end() const { return iterator(const_cast<vector*>(this)); }
        iterator rend() const { return end(); }
        iterator find(dword_t index) const {

            SegHelper helper(_allocator, _size);
            iterator ret(const_cast<vector *>(this));
            if (loader<T, 4, fast>::exist(_data, index, helper)) {
                ret._index = index;
                ret._valid = true;
            }
            return ret;
        }
        void erase(iterator it) { if ( it._valid ) erase( it._index ); }

        T &operator [] (dword_t index) {
            SegHelper helper(_allocator, _size);
            return loader<T, 4, fast>::load(_data, index, helper);
        }
        void erase(dword_t index) { 
            if ( _data ) {
                SegHelper helper(_allocator, _size);
                loader<T, 4, fast>::destroy(_data, index, helper); 
            }
        }
        dword_t size() const { return _size; }
    };
}

