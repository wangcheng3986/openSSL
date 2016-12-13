//
// Created by Administrator on 2016/12/6.
//

#include <stddef.h>
#include "ReqQueue.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

static void parse_head(RequestQueue* rq,const char *buf, int len);
static int parsed(RequestQueue* rq);


static void parse_head(RequestQueue* rq,const char *buf, int len){
    if (rq->requestHeader != NULL){
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

    int index = tmpStr - buf - 4;
    if(index > 0){
        rq->requestHeader = (char*)malloc(index);
        memset(rq->requestHeader,0, index);
        memcpy(rq->requestHeader, buf, index);
        flog(rq->requestHeader);
        rq->_state = http_content;
        flog("req_parse_head");
    }
}


static int parsed(RequestQueue* rq){
    if ( rq->_state == http_head) {
        return 0;
    }
    return 1;
}


RequestQueue* create_req(){
    RequestQueue* rq = (RequestQueue*)malloc(sizeof(RequestQueue));
    memset(rq,0, sizeof(RequestQueue));
    return rq;
}
void destroy_req(RequestQueue* rq){
    if (rq != NULL){
        if(rq->requestHeader != NULL){
            free(rq->requestHeader);
        }
        free(rq);
        rq = NULL;
    }
}

void push_req(RequestQueue* rq, const char *buf, int len){
    if(rq == NULL || buf == NULL){
        return;
    }
    switch ( rq->_state )
    {
        case http_head:{
            flog("push_req:Protocol_head");
            parse_head(rq,buf, len);
            break;
        }
        case http_content:
        case http_end:
        default:
            flog("push_req:default");
            break;
    }
    rq->_upsize += len;
}