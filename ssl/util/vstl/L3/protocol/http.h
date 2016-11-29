#pragma once
#include "../../L0/abstract/allocator.h"
#include "../../L0/abstract/type.h"
#include "../../L0/containers/string.h"
#include "../../L2/buffer/stream.h"
#include "../../L2/buffer/buffer.h"
#include "../../L2/containers/map.h"
//#include "../../L2/allocator/wrapper_allocator.h"
#include "../../L2/containers/string.h"

#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <bits/sockaddr.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#else
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif
namespace protocol3_http {

    using namespace type_l0;

    typedef container_l2::string string;
    typedef container_l2::map<string, string> string_map;

    static
    const char *get_val(string_map &strings, const char *name) {
        for ( string_map::iterator it = strings.begin(); it != strings.end(); ++it ) {
            if ( strcasecmp(it->first.c_str(), name) == 0 ) return it->second.c_str();
        }
        return 0;
    }
    class request_encoder :public allocator_l0::contain_base{
        string _uri;
        string _method;
        string _ver;
        string_map _params;
        string_map _cookies;
    public:
        request_encoder(allocator_l0::allocator *Allocate=0)
            : _uri("/", Allocate)
            , _method("GET", Allocate)
            , _ver("HTTP/1.1", Allocate)
            , _params(Allocate)
            , _cookies(Allocate)
        {
        }
        request_encoder(const request_encoder &right):_uri(right._uri), _method(right._method), _ver(right._ver), _params(right._params.get_allocator()) {
            _params = right._params;
        }
        request_encoder & operator = ( const request_encoder &right) {
            if ( _params.get_allocator() == 0 ) {
                _params.set_allocator(right._params.get_allocator());
                _uri.set_allocator(right._params.get_allocator());
                _method.set_allocator(right._params.get_allocator());
                _ver.set_allocator(right._params.get_allocator());
            }
            _params = right._params;
            _uri = right._uri;
            _method = right._method;
            _ver = right._ver;
            return *this;
        }
        string_map &params() { return _params; }
        string_map &cookies() { return _cookies; }
        void set_method(const char *method) { _method = method;}
        void set_uri(const char *uri) { _uri = uri; }
        void set_ver(const char *ver) { _ver = ver; }
        void default_init() {
   			_params[string("Accept", _params.get_allocator())] = "*/*";
			_params[string("Accept-Language", _params.get_allocator())] = "zh-cn";
			_params[string("Accept-Encoding", _params.get_allocator())] = "gzip, deflate";
			_params[string("User-Agent", _params.get_allocator())] = "HTTP-TOOL";
			_params[string("Content-Length", _params.get_allocator())] = "0";
			_params[string("Connection", _params.get_allocator())] = "Close";
        }
        dword_t encode(char *buffer, dword_t len) {

            if ( len < _method.size() + _uri.size() + _ver.size() + 7 ) return 0;
            dword_t ret = sprintf(buffer, "%s %s %s\r\n", _method.c_str(), _uri.c_str(), _ver.c_str());
            for ( string_map::iterator it = _params.begin(); it != _params.end(); ++it ) {
                if ( len < ret + it->first.size() + it->second.size() + 2 + 4 + 1 ) return 0;
                ret += sprintf(buffer + ret, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
            }

            if ( _cookies.size() ) {
                if ( len < ret + 20 ) return 0;
                ret += sprintf(buffer + ret, "Cookie: ");
                for ( string_map::iterator it = _cookies.begin(); it != _cookies.end(); ++it ) {
                    if ( len < ret + it->first.size() + it->second.size() + 3 + 4 + 1 ) return 0;
                    ret += sprintf(buffer + ret, "%s=%s; ", it->first.c_str(), it->second.c_str());
                }
                ret += sprintf(buffer + ret, "\r\n");
            }
            ret += sprintf(buffer + ret, "\r\n");
            return ret;
        }
    };
    class response_decoder :public allocator_l0::contain_base{
        string _ver;
        word_t _rcode;//init 0
        string_map _params;
        string_map _cookies;
        bool _parsed;//init false
        typedef container_l2::stream<1000> my_stream;
        my_stream _response;
        my_stream _header;
        long long _content_len;//default -1
        qword_t _recved;
        bool _protocol_error; //init false
    public:
        enum transfer_mode {
            fixed = 0,
            ranged,
            chunked,
            byclose
        };
        transfer_mode trans_mode() const { return _mode; }
        class ChunkModule {
            my_stream &_response;
            char _chunk_cache[64];
            dword_t _chunk_info_size;
            dword_t _chunk_size;
            dword_t _chunk_transfer;
            qword_t _response_size;
            bool _trunk_recved;
            bool _error;
            dword_t parse_chunk(const char *buffer, dword_t len, bool &b_parsed) {

                b_parsed = false;
                dword_t cp_size = sizeof(_chunk_cache) - 1 - _chunk_info_size;
                cp_size = unistd_l0::Min(cp_size, len);
                memcpy(_chunk_cache + _chunk_info_size, buffer, cp_size);
                _chunk_cache[_chunk_info_size + cp_size] = '\0';
                if ( cp_size + _chunk_info_size <= 2 ) { _chunk_info_size += cp_size; return cp_size; }

                if ( memcmp(_chunk_cache, "\r\n", 2) ) {
                    _error = true; return 0;
                }
                char *end_tok = strstr(_chunk_cache + 2, "\r\n");
                if ( end_tok == 0 ) {
                    if ( _chunk_info_size + cp_size == sizeof(_chunk_info_size) - 1 ) _error = true;
                    return 0;
                }
                cp_size = (int)(end_tok + 2 - _chunk_cache - _chunk_info_size);
                *end_tok = '\0';
                _chunk_size = unistd_l0::atox<dword_t>(_chunk_cache + 2);
                b_parsed = true;
                _chunk_info_size = 0;
                _trunk_recved = true;
                return cp_size;
            }
        public:
            ChunkModule(my_stream &response) 
                : _response(response)
                , _chunk_info_size(0)
                , _chunk_size(0)
                , _chunk_transfer(0)
                , _response_size(0)
                , _trunk_recved(false)
                , _error(false)
            {
            }
            void reset() {
                _response.clear();
                _chunk_info_size = 0;
                _chunk_size = 0;
                _chunk_transfer = 0;
                _response_size = 0;
                _trunk_recved = false;
                _error = false;
            }
            ~ChunkModule(){}
            bool finished() const { return _chunk_size == 0 && _trunk_recved; }
            qword_t response_size() const { return _response_size; }
            bool error() const { return _error; }
            void push(const char *buffer, dword_t len) {
                if ( len == 0 ) return;
                if ( ! _trunk_recved && _chunk_info_size == 0 ) {
                    memcpy(_chunk_cache, "\r\n", 2); _chunk_info_size += 2;
                }
                while ( len ) {

                    if ( _chunk_transfer == _chunk_size ) {
                        _chunk_size = _chunk_transfer = 0;
                        bool b_parsed = false;
                        dword_t parse_size = parse_chunk(buffer, len, b_parsed);
                        if ( _error || (! b_parsed) || _chunk_size == 0 ) return;
                        buffer += parse_size; len -= parse_size;
                    }
                    dword_t chunk_left = _chunk_size - _chunk_transfer;
                    dword_t copysize = unistd_l0::Min(chunk_left, len);
                    _response.push_back(buffer, copysize);
                    _chunk_transfer += copysize;
                    _response_size += copysize;
                    buffer += copysize; len -= copysize;
                }
            }
        };
        struct ContentRange {
            long long start;
            long long end;
            long long full;
            ContentRange():start(0), end(0), full(0){}
            void reset() { start = end = full = 0; }
            ContentRange(long long Start, long long End, long long Full):start(Start), end(End), full(Full) {}
            long long length() { return end - start + 1;}
            bool init(const string &range_content) {
                if ( range_content.size() >= 100 ) return false;
                char buffer[100];
                strcpy(buffer, range_content.c_str());
                const char *head = "bytes ";
                if ( strstr(buffer, head) != buffer ) return false;
                char *pCode = buffer + strlen(head);
                char *pNext = 0;

                for ( int i = 0; pCode[i]; i++ ) {
                    if ( pCode[i] == '-' ) {
                        pCode[i] = '\0'; pNext = pCode + i + 1; break;
                    }
                }
                start = unistd_l0::atot<long long>(pCode);

                pCode = pNext;
                for ( int i = 0; pCode[i]; i++ ) {
                    if ( pCode[i] == '/' ) {
                        pCode[i] = '\0'; pNext = pCode + i + 1; break;
                    }
                }
                end = unistd_l0::atot<long long>(pCode);
                full = unistd_l0::atot<long long>(pNext);
                return true;
            }
        };

        string_map &params() { return _params; }
        string_map &cookies() { return _cookies; }
        const char *ver() const { return _ver.c_str(); }
        word_t rcode() const { return _rcode; }
        bool parsed() const { return _parsed; }
        qword_t recved() const {
            if ( ! _parsed ) return 0;
            if ( _mode == chunked ) return _chunk.response_size();
            return _recved;
        }
        long long ret_size() const {
            if ( ! _parsed ) return -1;
            if ( _mode == chunked ) return -1;
            return _content_len;
        }
        bool protocol_error() const { return _protocol_error || _chunk.error(); }
        dword_t parse(const container_l2::stream_wrapper &stream_data) {

            int offset = stream_data.find("\r\n\r\n", 4);
            if ( offset == -1 ) return 0;
            int head_size = offset + 4;
            //解析每一行
            container_l2::smart_buffer<char, 1024 * 10> buffer(_params.get_allocator(), offset + 5);
            stream_data.peek(buffer.buffer(), head_size);
            pushback(buffer.buffer(), head_size);
            return head_size;
        }
        void pushback(const char *buffer, dword_t len) {
            if ( ! _parsed ) {
                _header.push_back(buffer, len);
                dword_t all_size = _header.size();
                int offset = -1;
                my_stream::iterator it = _header.find("\r\n\r\n", 4, offset);
                if ( offset == -1 ) return;
                _parsehead(offset + 4);
                if ( _parsed ) {
                    dword_t data_len = all_size - offset - 4;
                    _header.clear();
                    pushback(buffer + (len - data_len), data_len);
                }
            }
            else {
                if ( _mode == chunked ) _chunk.push(buffer, len);
                else {
                    _response.push_back(buffer, len);
                    _recved += len;
                }
            }
        }
        dword_t popfront(char *buffer, dword_t len) {
            return _response.pop_front(len, buffer);
        }
        response_decoder(allocator_l0::allocator *Allocate = 0)
            : _ver(Allocate)
            , _rcode(0)
            , _params(Allocate)
            , _cookies(Allocate)
            , _parsed(false)
            , _response(Allocate)
            , _header(Allocate)
            , _content_len(-1)
            , _protocol_error(false)
            , _mode(byclose)
            , _chunk(_response)
            , _recved(0)
        {
        }
        ~response_decoder(){}
        void reset() {
            _ver.clear();
            _rcode = 0;
            _params.clear();
            _cookies.clear();
            _parsed = false;
            _response.clear();
            _header.clear();
            _content_len = -1;
            _protocol_error = false;
            _mode = byclose;
            _chunk.reset();
            _range.reset();
            _recved = 0;
        }
    private:
        transfer_mode _mode;
        ContentRange _range;
        ChunkModule _chunk;
        bool _parse_protocol(int len) {
            container_l2::smart_buffer<char, 1024> buffer(_header.get_allocator(), len);
            _header.pop_front(len, buffer.buffer());
            buffer[len - 2] = '\0';

            char *http_head = strstr(buffer.buffer(), "HTTP/");
            if ( http_head ) {
                //取命令头,应答代码
                char *pCode = strchr(http_head, ' ');
                if ( ! pCode ) return false;
                _ver.assign(http_head, pCode - http_head);

                if ( http_head == buffer.buffer() ) {
                    for ( pCode++ ;*pCode == ' '; pCode++ );
                    char *pCodeEnd = strchr(pCode, ' ');
                    if ( ! pCodeEnd ) return false;
                    *pCodeEnd = '\0'; 
                    _rcode = atoi(pCode);
                }
                else if ( strstr(buffer.buffer(), "NOTIFY") ) _rcode = 200;
                else return false;

                return true;
            }
            return false;
        }
        bool _parse_param(int len) {

            container_l2::smart_buffer<char, 1024> buffer(_header.get_allocator(), len);
            _header.pop_front(len, buffer.buffer());
            buffer[len - 2] = '\0';

            const char *split_tok = strstr(buffer.buffer(), ":");
            if ( split_tok == 0 ) return false;
            string name(_header.get_allocator());
            name.assign(buffer.buffer(), (int)(split_tok - buffer.buffer()));
            const char *value = split_tok + 1;
            while ( *value == ' ' ) value++;
            _params[name] = value;
            return true;
        }
        void _parsehead(int len) {

            int b_parsed = 0;
            int line_size;
            _header.find("\r\n", 2, line_size);
            if ( ! _parse_protocol(line_size + 2) ) {
                _protocol_error = true;  return;
            }
            b_parsed += line_size + 2;
            while ( b_parsed < len - 2) {

                _header.find("\r\n", 2, line_size);
                if ( ! _parse_param(line_size + 2) ) {
                    _protocol_error = true; return;
                }
                b_parsed += line_size + 2;
            }
            _header.pop_front(2);

            //传输模式识别
            //range模式
            //chunked模式
            //固定长度模式
            //以上都没有,服务器close决定(default)
            string name("Content-Length", _params.get_allocator());
            string_map::iterator it = _params.find(name);
            if ( it != _params.end() ) {
                _mode = fixed;
                _content_len = unistd_l0::atot<long long>(it->second.c_str());
            }
            else {
                name.assign("Content-Range");
                it = _params.find(name);
                if ( it != _params.end() && _rcode != 416 ) {
                    if ( _range.init(it->second) ) {
                        _mode = ranged;
                        _content_len = _range.length();
                    }
                }
                if ( _mode != ranged ) {
                    name.assign("Transfer-Encoding");
                    it = _params.find(name);
                    if ( it != _params.end() ) {
                        if ( strcasecmp(it->second.c_str(), "chunked") == 0 ) _mode = chunked;
                    }
                }
            }

            _parsed = true;
        }

    };

    class request_decoder :public allocator_l0::contain_base{

        string _uri;
        char _method[16];
        char _ver[12];
        dword_t _state;
        dword_t _head_size;
        string_map _params;

        bool _parse_protocol(char *buffer, size_t len)
        {
            _method[0] = _ver[0] = '\0';

            while (*buffer == ' ') ++buffer;

            char *it = strchr(buffer, ' ');
            if ( it == 0 ) return false;
            if ( it - buffer >= sizeof(_method) ) return false;
            char *uri = it + 1;
            while ( *uri == ' ' ) ++uri;
            if ( ! *uri ) return false;

            memcpy(_method, buffer, it - buffer);
            _method[it - buffer] = '\0';

            it = strchr(uri, ' ');
            _uri.assign(uri, it ? it - uri: strlen(uri));
            if ( it ) {
                while ( *it == ' ' ) ++it;
                char *ver = it;
                it = strchr(ver, ' ');
                size_t cp_size = unistd_l0::Min<dword_t>((dword_t)(it ?it - ver: strlen(ver)), (dword_t)(sizeof(_ver) - 1));
                memcpy(_ver, ver, cp_size);
                _ver[cp_size] = '\0';
            }
            return true;
        }

        bool _parse_pragma(char *buffer, size_t len) {

            const char *token = ": ";
            char *it = strstr(buffer, token);
            if ( ! it ) return false;
            allocator_l0::allocator *allocator_ = _params.get_allocator();
            string key;
            ::set_allocator(key, allocator_);
            key.assign(buffer, it - buffer);
            string value(it + 2, _params.get_allocator());
            _params.insert(key, value);
            return true;
        }

    public:
        request_decoder() { reset();}
        request_decoder(allocator_l0::allocator *Allocate): _params(Allocate), _uri(Allocate){reset();}
        request_decoder(const request_decoder &right): _params(right._params.get_allocator()), _uri(right._uri), _head_size(0), _state('f'){

            _params = right._params;
        }
        request_decoder & operator = ( const request_decoder &right) {
            if ( _params.get_allocator() == 0 ) {
                _params.set_allocator(right._params.get_allocator());
                _uri.set_allocator(right._params.get_allocator());
            }
            _head_size = right._head_size;
            _state = right._state;
            _params = right._params;
            _uri = right._uri;
            memcpy(_method, right._method, sizeof(_method));
            memcpy(_ver, right._ver, sizeof(_ver));
            return *this;
        }

        bool error() const { return _state == 'e'; }
        dword_t parse(const char *data, dword_t len) {}
        dword_t parse(const container_l2::stream_wrapper &stream_data) {

            int offset = stream_data.find("\r\n\r\n", 4);
            if ( offset == -1 ) return 0;
            _head_size = offset + 4;
            //解析每一行
            container_l2::smart_buffer<char, 1024 * 10> buffer(_params.get_allocator(), offset + 5);
            stream_data.peek(buffer.buffer(), _head_size - 2);
            buffer[_head_size - 2] = '\0';
            {
                const char *line_tok = "\r\n";
                int i = 0;
                char *line = buffer.buffer();

                for ( char *end ; end = strstr(line, line_tok) ; i++, line = end + 2 ) {

                    *end = '\0';
                    if ( i == 0 ) {

                        if ( ! this->_parse_protocol(line, end - line) ) {
                            _state = 'e';
                            _head_size = 0;
                            break;
                        }
                    }
                    else if ( ! this->_parse_pragma(line, end - line) ) {
                        _state = 'e';
                        _head_size = 0;
                        break;
                    }
                }
                return _head_size;
            }
            
        }

        const char *query(const char *qtype) const {

            //"uri", "method", "ver", "state", "count param"
            switch ( *qtype ) {
            case 'u':
                return _uri.c_str();
            case 'm':
                return _method[0] ? _method: 0;
            case 'v':
                return _ver[0] ? _ver: 0;
            default:
                return 0;
            }
        }
        const char *param(const char *name) const {

            string_map::iterator it = _params.find(string( name, _params.get_allocator() ));
            return ( it == _params.end() ) ? 0: it->second.c_str();
        }
        string_map &params() { return _params; }

        dword_t get(const char *qtype) const {

            //"head size", "parameter count", 
            switch ( *qtype ) {
            case 'h':
                return _head_size;
            default:
                return 0;
            }
        }
        void reset() {

            _uri.clear();
            _params.clear();
            _method[0] = _ver[0] = '\0';
            _state = 'f';
            _head_size = 0;
        }

        class iterator {
            string_map::iterator _it;
        public:
            iterator(string_map::iterator it):_it(it){}
            iterator(const iterator &right) :_it(right._it){}
            iterator &operator = (const iterator &right) { _it = right._it; return *this; }
            iterator &operator ++ () { ++_it;  return *this; }
            iterator &operator -- () { --_it;  return *this; }
            string_map::map_node &operator * () {  return *_it; }
            string_map::map_node &operator -> () { return *_it; }
            bool operator == (const iterator &right) { return _it == right._it; }
            bool operator != (const iterator &right) { return ! operator == (right); }
        };
        iterator end() const { return iterator(_params.end()); }
        iterator begin() { return iterator(_params.begin());}
        iterator find(const string &value) {
            return iterator(_params.find(value));
        }

    };

    class response_encoder :public allocator_l0::contain_base {

        string_map _params;
        word_t _rcode;

    public:
        response_encoder(allocator_l0::allocator *Allocate): _params(Allocate), _rcode(200){}

        dword_t encode(container_l2::stream_wrapper &stream_data) {

            dword_t exist_size = stream_data.size();
            //协议, 返回代码, 
            char buff[100]; 
            int size = sprintf(buff, "HTTP/1.1 %d\r\n", _rcode);
            stream_data.push_back(buff, size);
            for ( string_map::iterator it = _params.begin(); it != _params.end(); ++it ) {
                stream_data.push_back(it->first.c_str(), (dword_t)it->first.size());
                stream_data.push_back(": ", 2);
                stream_data.push_back(it->second.c_str(), (dword_t)it->second.size());
                stream_data.push_back("\r\n", 2);
            }
            stream_data.push_back("\r\n", 2);
            return stream_data.size() - exist_size;
        }
        void set_code(word_t code) { _rcode = code; }
        string &pragma(const char *name) {
            return _params[string(name, _params.get_allocator())];
        }

        const char *query(const char *name) const {

            string_map::iterator it = _params.find(string(name, _params.get_allocator()));
            return (it != _params.end())? it->second.c_str() :0;
        }

    };

    inline bool isIpString(const char *szTargetString)
    {

        static const int pMarray[] = {0, 0, 10, 100};

        for ( int i = 0; i < 4; ++i ) {

            int iByte = 0;

            int j = 0;

            for ( j = 0; j < 4; ++j, ++szTargetString ) {

                if ( *szTargetString == '\0' ) break;

                if ( *szTargetString == '.' ) {

                    if ( i == 3 ) return false;

                    ++szTargetString; break;

                }

                char chCurrentChar = *szTargetString - '0';

                if ( (unsigned char)chCurrentChar > 9 ) return false;

                if ( ( iByte = iByte * 10 + chCurrentChar ) > 255 ) return false;

            }

            if ( (j == 0) || (iByte < pMarray[j]) ) return false;

            if ( *szTargetString == '\0' ) return i == 3;

        }

        return false;

    }

    
    static bool get_name(const char *url, container_l0::const_string &name, const char *head_token, char end_token1, char end_token2) {

        char *httpHead = (char*)strstr(url, head_token);
        const char *host = (httpHead == url) ?httpHead + strlen(head_token): url;
        const char *host_end = strchr(host, end_token1);

        if ( (host_end == 0) && end_token2 ) host_end = strchr(host, end_token2);

        int len = host_end ? (int)(host_end - host) : (int)strlen(host);
        if ( len == 0 ) return false;
        name.assign(host, len);
        return true;
    }
    static bool getDomainName(const char *url, container_l0::const_string &domainName) {
        const char *protocol_type = "http://";
        if ( strstr(url, "https://") == url) protocol_type = "https://";
        return get_name(url, domainName, protocol_type, ':', '/');
    }
    static bool getHost(const char *url, container_l0::const_string &host) {
        const char *protocol_type = "http://";
        if ( strstr(url, "https://") == url) protocol_type = "https://";
        return get_name(url, host, protocol_type, '/', 0);
    }
    static const char * get_uri(const char *url) {
        container_l0::const_string host;
        if ( ! getHost(url, host) ) return 0;
        const char * retval = host.c_str() + host.size();
        if ( *retval == '/' ) return retval;
        return "/";
    }
    static bool getAddr(const char *url, dword_t &ip, word_t &port) {

        container_l0::const_string host;
        if ( ! getHost(url, host) ) return false;
        char strhost[256] = {0};
        if ( host.size() >= sizeof(strhost) ) return false;
        strncpy(strhost, host.c_str(), host.size());

        char *pPort = strstr(strhost, ":");
        if ( pPort ) {
            *pPort = '\0'; pPort++;
        }

        if ( isIpString(strhost) ) {
            ip = ntohl(inet_addr(strhost));
        }
        else {

//             addrinfo hints = {0};
//             addrinfo *res = 0;
//             hints.ai_family = AF_INET;
//             hints.ai_socktype = SOCK_STREAM;
//             if( getaddrinfo(strhost, 0, &hints, &res) ) {
//                 return false;
//             }
// 
//             freeaddrinfo(res);
            hostent * hostname = gethostbyname(strhost);
            if ( hostname == 0 ) return false;

            ip = ntohl(*((dword_t *)&hostname->h_addr_list[0][0]));
        }
        if ( pPort ) port = (word_t)atoi(pPort);
        else port = (strstr(url, "https://") == url)?443:80;
        return true;
    }

}
