//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_ENTITY_H
#define LOGIC_ENTITY_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <malloc.h>

#ifndef int64
#define int64 long long
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
    int64 _begin_time;
    int64 _end_time;
}SSL_NEW;

typedef struct {
    void *_ssl;
    int64 _begin_time;
}SSL_FREE;

typedef struct {
    void *_ssl;
    int _ret;
    int64 _begin_time;
    int64 _end_time;
}SSL_CONNECT;

typedef struct {
    void * _ssl;
    int _ret;
    int _len;
    int64 _begin_time;
    int64 _end_time;
    void *_buf;
}SSL_READ;



typedef struct {
    void * _ssl;
    int _ret;
    int _len;
    int64 _begin_time;
    int64 _end_time;
    void *_buf;
}SSL_WRITE;

typedef enum _HttpState {
    http_head = 0,
    http_content,
    http_end
}HttpState;


typedef struct _RequestHeader {
    int protocol;//0get,post
    char* pa;
    char* host;
}ReqHeader;

typedef struct _RequestQueue {
    char* strHeader;
    ReqHeader* reqHeader;
    int64 req_start_time;
    int64 req_end_time;
    int64 _upsize;
    HttpState _state;
}RequestQueue;


typedef struct _ResponseQueue {
    char* strHeader;
    int64 rsp_start_time;
    int64 rsp_end_time;
    long _downsize;
    int scode;
    long contentLength;
    long left;
    HttpState _state;
}ResponseQueue;

typedef struct  _ConnectionInfo{
    int64 connect_start;
    int64 connect_end;
    int _err_num;
    void *_ssl;
    int _connected;
    RequestQueue* reqQueue;
    ResponseQueue* rspQueue;
    struct _ConnectionInfo *next;
}ConnectionInfo;


#ifdef  __cplusplus
}
#endif


#endif //LOGIC_ENTITY_H
