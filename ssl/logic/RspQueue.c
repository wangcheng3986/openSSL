//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "RspQueue.h"
#include "log.h"


static void parse_rsp_head(ResponseQueue* rq, const char *buf, int len);

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
static void parse_rsp_head(ResponseQueue* rq, const char *buf, int len){
    if (rq->responseHeader != NULL){
        return;
    }
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

    int headLen = tmpStr - buf - 4;
    if(headLen > 0){
        rq->responseHeader = (char*)malloc(headLen);
        memset(rq->responseHeader,0, headLen);
        memcpy(rq->responseHeader, buf, headLen);
        flog(rq->responseHeader);
        rq->_state = http_content;
        flog("rsp_parse_head");
    }
    char log[256];
    sprintf(log, "--------on_down------------,%d,%d", headLen,len);
    flog(log);
}

void push_rsp(ResponseQueue* rq, const char *buf, int len){

    if(rq!= NULL){
        char log[256];
        sprintf(log, "--------push_rsp------------,%d",len);
        flog(log);
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