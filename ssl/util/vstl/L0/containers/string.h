#pragma once
namespace container_l0 {
    template <class T> 
    class base_string {
        T *_value;
        int _len;
    public:
        int len() const { return _len; }
        int size() const { return _len; }
        T *c_str() const { 
            if ( _len == 0 && _value == 0 ) return (T*)(&_value);
            return _value; 
        }
        base_string():_value(0), _len(0){}
        base_string(T *value, int len):_value(value), _len(len){}
        void assign(T *value, int len) { _value = value, _len = len; }
        T & operator [] (int index) { return _value[index]; }
        bool operator == ( const base_string &value) {
            if ( _len != value._len ) return false;
            for ( int i = 0; i < _len; ++i ) {

                if ( _value[i] != value._value[i] ) return false;
            }
            return true;
        }
        bool operator == (const T *value) {
            for ( int i = 0;i < _len; ++i ) {
                if ( _value[i] != value[i] ) return false;
            }
            return value[_len] == 0;
        }
    };
    typedef base_string<char> string;
    typedef base_string<const char> const_string;
}