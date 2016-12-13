//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "RspQueue.h"
#include "log.h"


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
    char *tmpStr = buf;
    char *substr = "\r\n";
    char *ret = NULL;
    while(tmpStr){
        char *s = strstr(tmpStr, substr);
        if (s != NULL){
            tmpStr = s+4;
        }else{
            break;
        }
    }

    int offLen = tmpStr - buf;
    flog("rsp_parse_head:");
    if(offLen > 0){
        if (rq->responseHeader == 0){
            flog("rsp_parse_head:1");
            rq->responseHeader = (char*)malloc(2000);
            memset(rq->responseHeader,0, 2000);
            memcpy(rq->responseHeader, buf, index);
        }else{
            flog("rsp_parse_head:2");
            int exist = strlen(rq->responseHeader);
            if(exist+index < 2000){
                memcpy(rq->responseHeader + exist, buf, index);
            }
        }
        flog(rq->responseHeader);
    }
}

void push_rsp(ResponseQueue* rq, const char *buf, int len){
    if(rq!= NULL){
        switch ( rq->_state )
        {
            case http_head:
                parse_rsp_head(rq, buf, len);
                break;
            case http_content:
                break;
            case http_end:
                return ;
                break;
        }
        rq->_downsize += len;
    }
}