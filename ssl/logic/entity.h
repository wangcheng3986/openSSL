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

typedef struct  _ConnectionInfo{
    UINT64 start_time;
    UINT64 connect_start;
    UINT64 connect_end;
    UINT64 curr_time;
    int _err_num;
    void *_ssl;
    bool _use_ssl;
    int _ref_count;
    bool _connected;

    struct _ConnectionInfo *next;
}ConnectionInfo;

#ifdef  __cplusplus
}
#endif


#endif //LOGIC_ENTITY_H
