#pragma once

#include "helper.h"
#include "../util/vstl/L2/containers/list.h"
#include "../util/vstl/L3/protocol/http.h"
#include "hook_callback.h"
#ifndef DWORD
#define DWORD unsigned int
#endif
using namespace type_l0;

class ChunkModule1 {
    enum ChunkState {
        chunk_head = 0,
        chunk_content,
        chunk_content_end,
        chunk_done,
        chunk_error
    };
    int _chunk_count;
    int _cache_size;
    int _chunk_size;
    int _chunk_left;
    type_l0::qword_t _response_size;
    char _chunk_cache[20];
    ChunkState _state;

    int parse_end(const char *buffer, int len) {
        assert(_cache_size < 2);
        if ( len + _cache_size < 2 ) {
            if ( len ) {
                _chunk_cache[0] = buffer[0];
                _cache_size += len;
            }
            return len;
        }
        int ret = 2 - _cache_size;
        memcpy(_chunk_cache + _cache_size, buffer, ret);

        if ( strncmp(_chunk_cache, "\r\n", 2) ) {
            _state = chunk_error;
            return 0;
        }
        _cache_size = 0;
        _state = _chunk_size ? chunk_head: chunk_done;
        return ret;
    }
    int parse_head(const char *buffer, int len, bool &b_parsed) {

        b_parsed = false;
        int ret = sizeof(_chunk_cache) - 1 - _cache_size; 

        ret = unistd_l0::Min(ret, len);
        memcpy(_chunk_cache + _cache_size, buffer, ret);
        _chunk_cache[_cache_size + ret] = '\0';
        if ( ret + _cache_size <= 2 ) { 
            _cache_size += ret; 
            return ret; 
        }
        char *end_tok = strstr(_chunk_cache, "\r\n");
        if ( end_tok == 0 ) {
            if ( _cache_size + ret == sizeof(_chunk_cache) - 1 ) {
                _state = chunk_error;
            }
            _cache_size += ret;
            return ret;
        }
        ret = (int)(end_tok + 2 - _chunk_cache - _cache_size);
        *end_tok = '\0';
        _chunk_size = _chunk_left = unistd_l0::atox<type_l0::dword_t>(_chunk_cache);
        _chunk_count++;
        _state = chunk_content;
        _cache_size = 0;
        b_parsed = true;
        return ret;
    }
public:
    ChunkModule1() { reset(); }
    ~ChunkModule1(){}
    void reset() {
        _response_size = _chunk_size = _cache_size = _chunk_left = _chunk_count = 0;
        _state = chunk_head;
    }
    int chunk_count() const { return _chunk_count; }
    bool finished() const { return _state == chunk_done;}
    bool error() const { return _state == chunk_error; }
    qword_t response_size() const { return _response_size; }
    int push(const char *buffer, int len) {

        int in_len = len;
        while ( len ) {
            int size = 0;
            bool parsed = false;
            switch ( _state )
            {
            case chunk_head:
                size = parse_head(buffer, len, parsed);//����_chunk_size �� _chunk_left
                if ( error() || ! parsed ) return in_len - len;
                break;
            case chunk_content:
                size = unistd_l0::Min<int>(len, _chunk_left);
                _chunk_left -= size;
                _response_size += size;
                if ( _chunk_left == 0 ) _state = chunk_content_end;
                break;
            case chunk_content_end:
                size = parse_end(buffer, len);
                if ( error() ) return in_len - len;
                break;
            case chunk_done:
                return in_len - len;
            case chunk_error:
                return in_len - len;
            default:
                assert(false);
                break;
            }
            buffer += size;
            len -= size;
        }
        return in_len - len;
    }
};


struct RequestInfo :public allocator_l0::contain_base {
    container_l2::string request;
    UINT64 start_time;
    UINT64 req_end_time;
    UINT64 response_time;
    UINT64 recved;
    UINT64 upload;
    RequestInfo():start_time(0), recved(0), response_time(0), upload(0), req_end_time(0){}
    RequestInfo(allocator_l0::allocator *Allocate):start_time(0), request(Allocate), recved(0), response_time(0), upload(0), req_end_time(0){}
    RequestInfo(const RequestInfo &_right) {
        set_allocator(_right.get_allocator());
        request.set_allocator(get_allocator());
        request = _right.request;
        start_time = _right.start_time;
        response_time = _right.response_time;
        req_end_time = _right.req_end_time;
        recved = _right.recved;
        upload = _right.upload;
    }
};
static void set_allocator(RequestInfo &val, allocator_l0::allocator *Allocate) {
    val.request.set_allocator(Allocate);
    val.set_allocator(Allocate);
}

typedef container_l2::list<RequestInfo> RequestList;
struct RequestQueue {
    enum ProtocolState {
        no_parsed = 0,
        http_head,
        http_content,
        not_http
    };

    RequestList _reqs;
    UINT64 _upsize;
    container_l2::stream<1000> _cache;
    ProtocolState _state;
    int _content_left;
    RequestQueue():_upsize(0), _state(no_parsed), _content_left(0){}
    void push(const char *buf, int len, UINT64 curr_time);
    int pre_parse(const char *buf, int len);
    int parse_head(const char *buf, int len, UINT64 curr_time);
    bool parsed() const {
        switch ( _state ) 
        {
        case http_head:
        case http_content:
        case not_http:
            return true;
        case no_parsed: 
            return false;
        default:
            assert(false);
            return false;
        }
    }
    bool is_http() const { 
        switch ( _state ) 
        {
        case http_head:
        case http_content:
            return true;
        case not_http:
        case no_parsed: 
            return false;
        default:
            assert(false);
            return false;
        }
    }
};

struct ResponseQueue {
    RequestQueue *_requests;
    container_l2::string response;
    container_l2::stream<1000> _cache;
    protocol3_http::response_decoder _res;
    ChunkModule1 _chunk;

    UINT64 _downsize;
    int _content_left;
    enum HttpState {
        http_head = 0,
        http_content,
        protocol_error
    };
    HttpState _state;
    bool _chunked;
    ResponseQueue():_chunked(false), _content_left(0), _state(http_head), _downsize(0){}
    void reset() {
        _chunked = false;
        _content_left = 0;
        if ( _state != protocol_error )
            _state = http_head;
        _downsize = 0;
        response.clear();
        _cache.clear();
        _chunk.reset();
    }
    int push(const char *buf, int len, UINT64 curr_time);
    int parse_head(const char *buf, int len, UINT64 curr_time);
};
struct IpAddrInfo {
    container_l2::string ip;
    int port;
    int ip_ver;
};
struct ConnectionInfo :public allocator_l0::contain_base {
    RequestQueue _requests;
    ResponseQueue _response;
    ChunkModule1 _chunk;
//     union {
//         sockaddr_in ipv4;
//         sockaddr_in6 ipv6;
//     } addr;
    IpAddrInfo addr;
    UINT64 start_time;
    UINT64 connect_start;
    UINT64 connect_end;
    UINT64 curr_time;
    int _err_num;
    int _fd;
    void *_ssl;
    bool _use_ssl;
    int _ref_count;
    hook_callback::logic_handler *_instance;
    bool _connected;
    ConnectionInfo(allocator_l0::allocator *Allocator=0)
        : start_time(0)
        , connect_start(0)
        , connect_end(0)
        , _err_num(0)
        , curr_time(0)
        , _connected(false)
        , _fd(-1)
        , _ssl(0)
        , _use_ssl(false)
        , _ref_count(1)

    {set_allocator(Allocator); _response._requests = &_requests; }
    void on_up(const char *buf, size_t len);
    void on_down(int fd, const char *buf, size_t len);
    void report(int fd);

    void on_user_close(int fd, int result_code);
};

typedef container_l2::map<int, ConnectionInfo*> _ConnMap;
typedef container_l2::map<void *, ConnectionInfo> _SSLMap;

bool get_ip_info(const struct sockaddr *sa, IpAddrInfo &addr);