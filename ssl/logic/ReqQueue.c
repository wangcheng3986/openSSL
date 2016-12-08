//
// Created by Administrator on 2016/12/6.
//

#include <stddef.h>
#include "ReqQueue.h"
#include "stream.h"
#include "ReqInfo.h"

#define Min(a,b) (( a < b ) ? a: b)
static int pre_parse(RequestQueue* rq,const char *buf, int len);
static int parse_head(RequestQueue* rq,const char *buf, int len, UINT64 curr_time);
static int parsed(RequestQueue* rq);


static int pre_parse(RequestQueue* rq,const char *buf, int len){
//    stream_push_back(rq->_cache,buf, len);
//    char head_buf[8] = {0};
//    int head_size = stream_peek(rq->_cache,head_buf, 4, 0);
//    if ( head_size < 4 ) return len;
//
//    do {
//
//        if ( strncmp(head_buf, "GET", 3) && strncmp(head_buf, "POST", 4) && strncmp(head_buf, "OPTI", 4) )
//            break;
//
//        container_l2::my_stream_wrapper<1000> wrapper(_cache);
//        protocol3_http::request_decoder req_decoder(0);
//        req_decoder.parse(wrapper);
//        int head_size = req_decoder.get("head size");
//        if ( req_decoder.error() )
//            break;
//        if ( head_size == 0 ) {
//            if ( _cache.size() > 1024 * 10 )
//                break;
//            return len;
//        }
//
//        if ( req_decoder.query("ver") == 0 ) break;
//        if ( strncmp(req_decoder.query("ver"), "HTTP/", 5) ) break;
//
//        rq->_state = http_head;
//        stream_pop_back(rq->_cache,len,0);
//        return 0;
//
//    } while (0);
//
//    stream_clear(rq->_cache);
//    rq->_state = not_http;
//    return len;
    return 0;
}

static int parse_head(RequestQueue* rq,const char *buf, int len, UINT64 curr_time){
//    _cache.push_back(buf, len);
//    container_l2::my_stream_wrapper<1000> wrapper(_cache);
//    protocol3_http::request_decoder req_decoder(0);
//    req_decoder.parse(wrapper);
//
//    int head_size = req_decoder.get("head size");
//    if ( head_size == 0 ) {
////        LOGD("head_size = 0\n");
//        return len;
//    }
//
//    const char *content_length = protocol3_http::get_val(req_decoder.params(), "Content-Length");
//    if ( content_length == 0 ) {
//        _content_left = strncasecmp(req_decoder.query("method"), "GET", 3)? -1: 0;
//    }
//    else {
//        _content_left = atoi(content_length);
//    }
//    container_l2::smart_buffer<char, 1024 * 10> buffer(head_size + 10);
//    _cache.pop_front(head_size, buffer.buffer());
//    RequestInfo info;
//    if ( _content_left > 0 ) info.upload = _content_left;
//    info.request.assign(buffer.buffer(), head_size);
//    info.start_time = info.req_end_time = curr_time;
//    _reqs.push_back(info);
////    LOGD("_reqs.size() = %d\n", _reqs.size());
//    int ret = len - _cache.size();
//    _cache.clear();
//    _state = http_content;

    return 0;//ret;
}

static int parsed(RequestQueue* rq){
    switch ( rq->_state )
    {
        case Protocol_head:
        case Protocol_content:
        case not_http:
            return 1;
        case no_parsed:
            return 0;
        default:
            return 0;
    }
}

int is_req_http(RequestQueue* rq) {
    switch ( rq->_state )
    {
        case Protocol_head:
        case Protocol_content:
            return 1;
        case not_http:
        case no_parsed:
            return 0;
        default:
            return 0;
    }
}


RequestQueue* create_req(){
    RequestQueue* rq = (RequestQueue*)malloc(sizeof(RequestQueue));
    memset(rq,0, sizeof(RequestQueue));
    rq->_cache = stream_create();
    rq->_reqs = ReqInfo_create();
    return rq;
}
void destroy_req(RequestQueue* rq){
    if (rq != NULL){
        steam_destroy(rq->_cache);
        ReqInfo_destroy(rq->_reqs);
    }
}

void push_req(RequestQueue* rq, const char *buf, int len, UINT64 curr_time){
    if(rq == NULL){
        return;
    }
    while ( len ) {
        int size = 0;
        switch ( rq->_state )
        {
            case no_parsed:{
                size = pre_parse(rq,buf, len);
                break;
             }

            case Protocol_head:{
                size = parse_head(rq,buf, len, curr_time);
                break;
            }

            case Protocol_content:{
                if ( rq->_content_left >=0 )
                {
                    size = Min(rq->_content_left, len);
                    rq->_content_left -= size;
                    if (rq-> _content_left == 0 ) {
                        rq->_state = http_head;
                    }
                }
                else
                {
                    size = len;
                }
                break;
            }

            case not_http:
                size = len;
                break;

            default:
                size = len;
                break;
        }
        buf += size;
        len -= size;
        rq->_upsize += size;
    }
}