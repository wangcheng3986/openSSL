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
    char *substr = "\r\n";
    char *s = strstr(buf, substr);
    if(s == NULL){
        if (rq->requestHeader == 0){
            rq->requestHeader = (char*)malloc(len);
            memcpy(rq->requestHeader,buf,len);
        }else{
            strcat(rq->requestHeader,buf);
        }
    }else{
        int offset = s- buf;
        if(offset > 0){
            int size = strlen(buf)- strlen(s);
            char* tmp = (char*)malloc(size);
            memcpy(tmp,buf,size);
            if (rq->requestHeader == 0){
                rq->requestHeader = tmp;
            }else{
                strcat(rq->requestHeader,tmp);
            }
            rq->_state = http_content;
            flog(rq->requestHeader);
        }
    };
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