//
// Created by Administrator on 2016/12/6.
//

#include <stddef.h>
#include "ReqQueue.h"
#include "log.h"
#include <stdio.h>
#include <string.h>


// 将str字符以spl分割,存于dst中，并返回子字符串数量
static int split(char dst[][80], char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    while( result != NULL )
    {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

static void getheader(RequestQueue* rq){
    if(rq->reqHeader == NULL){
        rq->reqHeader = (ReqHeader*)malloc(sizeof(ReqHeader));
        memset(rq->reqHeader, 0, sizeof(ReqHeader));
    }
    char *result = NULL;
    char* tmpHeader = (char*)malloc(strlen(rq->strHeader));
    memcpy(tmpHeader,rq->strHeader,strlen(rq->strHeader));

//    flog(rq->strHeader);
//    flog(tmpHeader);

    result = strtok(tmpHeader, "\r\n");
    char* protocol = NULL;
    char* host = NULL;
    while( result != NULL )
    {
        if(strstr(result, "GET")){
            rq->reqHeader->protocol = 0;
            protocol = result;
        }else if(strstr(result, "POST")){
            rq->reqHeader->protocol = 1;
            protocol = result;
        }else if(strstr(result, "Host")){
            host = result;
        }
        if(host && protocol){
            break;
        }
        result = strtok(NULL, "\r\n");
    }

    if(protocol){
        result = strtok(protocol, " ");
        int index = 0;
        while( result != NULL )
        {
            index++;
            if(index == 2){
                rq->reqHeader->pa = (char*)malloc(sizeof(char)* strlen(result));
                strcpy(rq->reqHeader->pa, result);
//                flog(rq->reqHeader->pa);
                break;
            }
            result = strtok(NULL, " ");
        }
    }
    if(host){
        result = strtok(host, " ");
        int index = 0;
        while( result != NULL )
        {
            index++;
            if(index == 2){
                rq->reqHeader->host = (char*)malloc(sizeof(char)* strlen(result));
                strcpy(rq->reqHeader->host, result);
//                flog(rq->reqHeader->host);
                break;
            }
            result = strtok(NULL, " ");
        }
    }
}


static void parse_head(RequestQueue* rq,const char *buf){
    if (rq->strHeader != NULL){
        return;
    }
    char *substr1 = "HTTP/1.1";
    char *substr2 = "\r\n";
    char *s1 = strstr(buf, substr1);
    char *s2 = strstr(buf, substr2);

    if(s1 && s2){
        char* tail = NULL;
        while(s2){
            tail = s2+4;
            if(tail){
                s2 = strstr(tail, substr2);
            }else{
                break;
            }
        }
        int len = 0;
        if(tail){
            len = tail - buf - 4;
        }else{
            len = strlen(buf);
        }

        rq->strHeader = (char*)malloc(len);
        memcpy(rq->strHeader,buf, len);
        rq->_state = http_content;
        getheader(rq);
    }

//    char log[256];
//    sprintf(log, "--------parse_REQ_head----_state--------%d", rq->_state);
//    flog(log);
}




RequestQueue* create_req(){
    RequestQueue* rq = (RequestQueue*)malloc(sizeof(RequestQueue));
    memset(rq,0, sizeof(RequestQueue));
    return rq;
}
void destroy_req(RequestQueue* rq){
    if (rq != NULL){
        if(rq->strHeader != NULL){
            free(rq->strHeader);
        }
        if(rq->reqHeader != NULL){
            if(rq->reqHeader->pa != NULL){
                free(rq->reqHeader->pa);
            }
            if(rq->reqHeader->host != NULL){
                free(rq->reqHeader->host);
            }
            free(rq->reqHeader);
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
//            flog("push_req:Protocol_head");
            parse_head(rq, buf);
            break;
        }
        case http_content:
        case http_end:
        default:
//            flog("push_req:default");
            break;
    }
    rq->_upsize += len;
}