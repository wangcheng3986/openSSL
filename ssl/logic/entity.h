//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_ENTITY_H
#define LOGIC_ENTITY_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif
typedef unsigned char byte_t;
typedef unsigned short word_t;
typedef unsigned int dword_t;
typedef unsigned long long qword_t;
#define node_size 1024
#define Min(a,b) a<b?a:b

typedef struct {
    void *_ctx;
    void *_ret;
    UINT64 _begin_time;
    UINT64 _end_time;
}SSL_NEW;

typedef struct {
    void *_ssl;
    UINT64 _begin_time;
}SSL_FREE;

typedef struct {
    void *_ssl;
    int _ret;
    UINT64 _begin_time;
    UINT64 _end_time;
}SSL_CONNECT;

typedef struct {
    void * _ssl;
    int _ret;
    int _len;
    UINT64 _begin_time;
    UINT64 _end_time;
    void *_buf;
}SSL_READ;



typedef struct {
    void * _ssl;
    int _ret;
    int _len;
    UINT64 _begin_time;
    UINT64 _end_time;
    void *_buf;
}SSL_WRITE;


typedef struct _node {
    struct _node *_next;
    struct _node *_pre;
    word_t _used;
    word_t _start;
    char _data[node_size];
}Node;

typedef struct _list{
    Node *_left;
    Node *_right;
    int _count;
}List;

typedef struct _stream{
    dword_t _used;
    List* _list;
}Stream;

typedef enum _HttpState {
    http_head = 0,
    http_content,
    protocol_error
}HttpState;

typedef enum _ProtocolState {
    no_parsed = 0,
    Protocol_head,
    Protocol_content,
    not_http
}ProtocolState;

typedef struct _RequestInfo {
    char* request;
    UINT64 start_time;
    UINT64 req_end_time;
    UINT64 response_time;
    UINT64 recved;
    UINT64 upload;
    struct _RequestInfo* next;
}RequestInfo;

typedef struct _RequestQueue {
    RequestInfo* _reqs;
    UINT64 _upsize;
    Stream* _cache;
    ProtocolState _state;
    int _content_left;
}RequestQueue;


typedef enum _ChunkState {
    chunk_head = 0,
    chunk_content,
    chunk_content_end,
    chunk_done,
    chunk_error
}ChunkState;

typedef struct _ChunkModule1 {

    int _chunk_count;
    int _cache_size;
    int _chunk_size;
    int _chunk_left;
    unsigned long long _response_size;
    char _chunk_cache[20];
    ChunkState _state;
}ChunkModule1;

typedef struct _ResponseQueue {
    RequestQueue *_requests;
    char* response;
    char* _cache;
//    protocol3_http::response_decoder _res;
    ChunkModule1 _chunk;

    UINT64 _downsize;
    int _content_left;

    HttpState _state;
    int _chunked;

}ResponseQueue;

typedef struct  _ConnectionInfo{
    UINT64 start_time;
    UINT64 connect_start;
    UINT64 connect_end;
    UINT64 curr_time;
    int _err_num;
    void *_ssl;
    int _ref_count;
    int _connected;
    RequestQueue* reqQueue;
    ResponseQueue* rspQueue;

    struct _ConnectionInfo *next;
}ConnectionInfo;



typedef struct _request_decoder {

    char* _uri;
    char _method[16];
    char _ver[12];
    dword_t _state;
    dword_t _head_size;
   // string_map _params;
}request_decoder;

#ifdef  __cplusplus
}
#endif


#endif //LOGIC_ENTITY_H
