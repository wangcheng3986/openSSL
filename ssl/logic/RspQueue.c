//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "RspQueue.h"

#define Min(a,b) (( a < b ) ? a: b)

static int parse_rsp_head(ResponseQueue* rq, const char *buf, int len, UINT64 curr_time);

ResponseQueue* create_rsq(){
    ResponseQueue* rsp = (ResponseQueue*)malloc(sizeof(ResponseQueue));
    memset(rsp,0, sizeof(ResponseQueue));
    return rsp;
}

static int parse_rsp_head(ResponseQueue* rq, const char *buf, int len, UINT64 curr_time){

}

int push_rsp(ResponseQueue* rq, const char *buf, int len, UINT64 curr_time){
    if(rq!= NULL){
        int ret = 0;
        int size = 0;
        switch ( rq->_state )
        {
            case http_head:
                if ( strlen(rq->response) ) return ret;
                size = parse_rsp_head(rq, buf, len, curr_time);
                break;
            case http_content:
                if (  rq->_chunked == 0)
                {
                    if ( rq->_content_left >=0 )
                    {
                        size = Min(rq->_content_left , len );

                        rq->_content_left -= size;
                        if ( rq->_content_left == 0 ) rq->_state = http_head;
                    }
                    else
                    {
                        size = len;
                    }
                }
                else
                {
                    const char *chunk_buf = buf;
                    int chunk_len = len;
                    while ( chunk_len ) {

//                        dword_t chunk_size = rq->_chunk.push(buf, len);
//                        chunk_len -= chunk_size;
//                        chunk_buf += chunk_size;
//                        size += chunk_size;
//                        if ( _chunk.error() ) {
//                            _state = protocol_error;
////                        printf("chunk error!!!!\n");
//                            break;
//                        }
//                        if ( _chunk.finished() ) {
////                        printf("chunk finished, chunkcount=%d, responsesize=%lld\n", _chunk.chunk_count(), _chunk.response_size());
//                            _content_left = 0;
//                            _chunked = false;
//                            if (_requests->_reqs.size() )
//                                _requests->_reqs.front().recved = _chunk.response_size();
//                            _chunk.reset();
//                            _state = http_head;
//                            break;
//                        }
                    }
                }
                break;
            case protocol_error:
                return ret + len;
                break;
        }
        buf += size;
        len -= size;
        rq->_downsize += size;
        ret += size;
        return ret;
    }
}