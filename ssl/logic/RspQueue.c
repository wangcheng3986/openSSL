//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "RspQueue.h"

#define Min(a,b) (( a < b ) ? a: b)

static int parse_rsp_head(ResponseQueue* rq, const char *buf, int len);

ResponseQueue* create_rsp(RequestQueue* rq){
    if(rq != NULL){
        ResponseQueue* rsp = (ResponseQueue*)malloc(sizeof(ResponseQueue));
        memset(rsp,0, sizeof(ResponseQueue));
        rsp->_req = rq;
        return rsp;
    }
    return NULL;
}
void destroy_rsp(ResponseQueue* rsp){
    if (rsp != NULL){
        if(rsp->responseHeader != NULL){
            free(rsp->responseHeader);
        }
        free(rsp);
        rsp = NULL;
    }
}
static int parse_rsp_head(ResponseQueue* rq, const char *buf, int len){
    char *substr = "\r\n";
    char *s = strstr(buf, substr);
    if(s == NULL){
        if (rq->responseHeader == 0){
            rq->responseHeader = (char*)malloc(len);
            memcpy(rq->responseHeader,buf,len);
        }else{
            strcat(rq->responseHeader,buf);
        }
    }else{
        int offset = s- buf;
        if(offset > 0){
            int size = strlen(buf)- strlen(s);
            char* tmp = (char*)malloc(size);
            memcpy(tmp,buf,size);
            if (rq->responseHeader == 0){
                rq->responseHeader = tmp;
            }else{
                strcat(rq->responseHeader,tmp);
            }
            rq->_state = http_content;
            rq->_contentleft = rq->_req->_contentlength - strlen(s)+ 4;
        }
    };
}

void push_rsp(ResponseQueue* rq, const char *buf, int len){
    if(rq!= NULL){
        int rvSize = 0;
        switch ( rq->_state )
        {
            case http_head:
                rvSize = parse_rsp_head(rq, buf, len);
                break;
            case http_content:
                break;
            case http_end:
                return ;
                break;
        }
        rq->_downsize += size;
        rq->_contentleft = rq->_req->_contentlength - rvSize;
        if(rq->_contentleft<=0){
            rq->_state = http_end;
        }
    }
}