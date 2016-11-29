#pragma once
#include "../../L0/abstract/unistd.h"
#include "../../L0/containers/string.h"
namespace protocol1_json {
using namespace unistd_l0;
    //返回串首
    static
    const char *parse_string(const char *data, int &len) {
        //跳过空白字符
        while ( is_space(*data) ) data++;
        //第一个字符必须是双引号
        if ( *data != '\"' ) {
            len = 0; return 0;
        }
        const char *retval = data + 1;
        for ( data++ ; *data && *data != '\"'; data++ ) {
            //跳过转义符
            if ( *data == '\\' ) {
                data++; 
                //字符串搜索结束，还没发现双引号
                if ( *data == '\0' ) {
                    len = 0; return 0;
                }
            }
        }
        if ( *data == '\0' ) {
            len = 0; return 0;
        }
        len = (int)(data - retval);
        return retval;
    }

    struct jbase {
        const char *_json;
        char _type;
        bool _error;
        void parse(const char *data) {
            _json = data;
            _error = false;
            while ( is_space(*_json) ) _json++;

            switch ( *_json ) {
            case '{'://值是对象
                _type = 'o';
                break;
            case '['://值是array
                _type = 'a';
                break;
            case '\"'://值是string
                _type = 's';
                break;
            case 'n'://值是null
            case 'N'://值是null
                if ( strncasecmp(_json, "null", 4) != 0 ) _error = true;
                _type = 'n';
                break;
            case 't'://值是true(boolean)
            case 'T'://值是true(boolean)
                if ( strncasecmp(_json, "true", 4) != 0 ) _error = true;
                _type = 'b';
                break;
            case 'f'://值是false(boolean)
            case 'F'://值是false(boolean)
                if ( strncasecmp(_json, "false", 5) != 0 ) _error = true;
                _type = 'b';
                break;
            default:
                //值是digit
                {
                    int len = 0;
                    const char *digit_str = parse_digit_string(data, len);
                    if (len == 0) _error = true;
                    if ( len == 1 ) {
                        _error = *digit_str == '-';
                    }
                    if (! _error ) _type = 'd';
                }
                break;
            }

        }
    };

    class jboolean :public jbase {
    public:
        jboolean(const char *data) { parse(data); }
        const char *end() {
            char tok = *_json | ('a' ^ 'A');
            if ( tok == 't' ) { return _json + 4; }
            else if ( tok == 'f' ) { return _json + 5; }
            return 0;
        }
        bool value() const { return (*_json | ('a' ^ 'A')) == 't'; }
    };

    class jnull :public jbase {
    public:
        jnull(const char *data) { parse(data); }
        const char *end() { return ( ( *_json | ('a' ^ 'A') ) == 'n' ) ? _json + 4 : 0; }
    };
    class jdigit :public jbase {
    public:
        jdigit(const char *data) { parse(data); }
        const char *end() {
            int len = 0;
            const char *digit_str = parse_digit_string(_json, len);
            if (len == 0) return 0;
            if ( len == 1 ) {
                if ( *digit_str == '-' ) return 0;
            }
            return digit_str + len;
        }
        double value() {

            char buffer[64];
            int len = 0;
            const char *digit_str = parse_digit_string(_json, len);
            if (len == 0) {
                _error = true; return 0.0;
            }
            if ( len == 1 ) {
                if ( *digit_str == '-' ) {
                    _error = true; return 0;
                }
            }
            if (len >= sizeof(buffer) ) len = sizeof(buffer) - 1;
            strncpy(buffer, digit_str, len);
            buffer[len] = '\0';

            return strtod(buffer, 0);
        }
    };

    class jstring :public jbase {
    public:
        jstring(const char *data){ parse(data); }
        const char *end() {
            int len = 0;
            const char *retval = parse_string(_json, len);
            if ( retval == 0 ) return 0;
            return retval + len + 1;
        }
        int read(char *buffer) {
            int size = 0;
            const char *data = parse_string(_json, size);
            if ( size ) {
                strncpy(buffer, data, size);
            }
            return size;
        }
    };

    class jobject;
    class jarray;
    static
    const char *get_end(jobject *);
    static
    const char *get_end(jarray *);

    class jvariant :public jbase{
    public:
        jvariant() {
            jbase::_json = 0;
            jbase::_type = -1;
            jbase::_error = true;
        }
        jvariant(const char *data) { parse(data); }
        const char *end() {
            switch(_type) {
            case 's':
                return ((jstring *)this)->end();
            case 'n':
                return ((jnull*)this)->end();
            case 'b':
                return ((jboolean*)this)->end();
            case 'd':
                return ((jdigit*)this)->end();
            case 'a':
                return get_end((jarray*)this);
            case 'o':
                return get_end((jobject*)this);
            default:
                return 0;
            }
        }
    };
    class jarray :public jbase {
    public:
        jarray(const char *data) { parse(data); }
        const char *next() {
            const char *object = _json + 1;
            while ( object ) {
                while ( is_space(*object) ) object++;
                if ( *object == ']') return object + 1;
                if ( *object == ',' ) object++;
                object = jvariant(object).end();
            }
            return 0;
        }

        struct iterator {
            jvariant _var;
            const char *_json;
            iterator(const char *data):_json(data),_var(data){}
            iterator():_json(0){}
            jvariant & operator*() { return _var; }
            bool operator != (const iterator &right) const { return ! (*this == right); }
            jvariant * operator->() { return (&**this); }

            bool operator == (const iterator &right) const {
                if ( _json == right._json ) return true;
                if ( ! (_json && right._json) ) return false;
                const char *me = _json; while ( is_space(*me) )me++;
                const char *you = right._json; while (is_space(*you))you++;
                return me == you;
            }

            iterator& operator++() {

                if ( _json ) _json = _var.end();
                if ( _json ) {

                    while ( is_space(*_json) ) _json++;
                    if ( *_json == ',' ) {
                        _json++;
                        _var.parse(_json);
                        if (_var._error) _json = 0;
                    }
                    else {
                        _json = 0;
                    }
                }
                return *this;
            }
        };
        iterator begin() { return iterator(_json + 1); }
        iterator end() { return iterator(); }

    };
    class jobject :public jbase {
    public:
        jobject(const char *data){ parse(data); }
        const char *next() {
            const char *ppaire = _json + 1;
            while ( ppaire ) {
                while ( is_space(*ppaire) ) ppaire++;
                if ( *ppaire == '}') return ppaire + 1;
                if ( *ppaire == ',' ) ppaire++;
                ppaire = paire(ppaire).end();
            }
            return 0;
        }

        struct paire {
            int _nlen;
            const char *_name;
            const char *_value;//指向当前值的指针
            paire(const char *data):_value(0) { parse(data); }
            paire():_name(0), _value(0), _nlen(0){ }
            const char *end() { return _value ? jvariant(_value).end() : 0; }

            bool parse(const char *data) {

                _name = parse_string(data, _nlen);
                if ( _name == 0 ) return false;
                data = _name + _nlen + 1;
                while ( is_space(*data) ) data++;
                if ( *data != ':' ) return false;
                _value = data + 1;
                return true;
            }
        };
        struct iterator {
            const char *_json;
            paire _pair;
            iterator(const char *data):_json(data),_pair(data) {}
            iterator():_json(0){}
            paire & operator*() { return _pair;}
            bool operator != (const iterator &right) const { return ! (*this == right); }
            paire * operator->() { return (&**this); }

            bool operator == (const iterator &right) const {
                if ( _json == right._json ) return true;
                if ( ! (_json && right._json) ) return false;
                const char *me = _json; while ( is_space(*me) )me++;
                const char *you = right._json; while (is_space(*you))you++;
                return me == you;
            }
            iterator& operator++() {

                if ( _json = _pair.end() ) {
                    while ( is_space(*_json) ) _json++;
                    if ( *_json == ',' ) {
                        _json++;
                        if ( ! _pair.parse(_json) ) _json = 0;
                    }
                    else _json = 0;
                }
                return *this;
            }
        };
        iterator begin() { return iterator(_json + 1); }
        iterator end() { return iterator(); }
    };
    static
    const char *get_end(jobject *obj) { return obj->next(); }
    static
    const char *get_end(jarray *obj_array) { return obj_array->next(); }

    static 
        bool read_value(jvariant &variant, char *str_buffer, int len)
    {
        if ( variant._type != 's' ) return false;
        int size = 0;
        const char *param_val = parse_string(variant._json, size);
        if ( param_val == 0 ) return false;
        if ( size > len - 1) size = len - 1;
        strncpy(str_buffer, param_val, size);
        str_buffer[size] = '\0';
        return true;
    }

    static 
        bool read_value(jvariant &variant, double &value)
    {
        if ( variant._type != 'd' ) return false;
        jdigit &digit_value = reinterpret_cast<jdigit &>(variant);
        if ( digit_value._error ) return false;
        value = digit_value.value();
        return true;
    }

    static 
        bool read_value(jvariant &variant, bool &value)
    {
        if ( variant._type != 'b' ) return false;
        jboolean &bool_value = reinterpret_cast<jboolean &>(variant);
        if ( bool_value._error ) return false;
        value = bool_value.value();
        return true;
    }
    static bool read_string(protocol1_json::jvariant &value, container_l0::const_string &str_val) {
        if ( value._type != 's' ) return false;
        int len = 0;
        const char *str = protocol1_json::parse_string(value._json, len);
        if ( str == 0 ) return false;
        str_val.assign(str, len);
        return true;
    }
}
