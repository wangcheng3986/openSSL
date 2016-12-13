//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "RspQueue.h"
#include <string.h>
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

static void parse_rsp_head(ResponseQueue* rq, const char *buf, int len);


// 将str字符以spl分割,存于dst中，并返回子字符串数量
int split(char dst[][80], char* str, const char* spl)
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

static void getheader(ResponseQueue* rq){
    if(rq->rspHeader == NULL){
        rq->rspHeader = (RspHeader*)malloc(sizeof(RspHeader));
        memset(rq->rspHeader, 0, sizeof(RspHeader));
    }
    char dst[10][80];
    int cnt = split(dst, rq->strHeader, "\r\n");
    int i = 0;
    for (; i < cnt; i++)
    {
        flog(dst[i]);
        char *s = strstr(dst[i], "HTTP/1.1");
        if(s != NULL){
            char list[5][80];
            int ll = split(list, dst[i], " ");
            if(ll > 2){
                rq->rspHeader->scode = atoi(list[1]);
            }
        }else{
            s = strstr(dst[i], "Content-Length");
            if(s != NULL){
                char list[3][80];
                int ll = split(list, dst[i], " ");
                if(ll == 2){
                    rq->rspHeader->contentLength = atol(list[1]);
                }
            }
        }
    }
    char log[256];
    sprintf(log, "--------getheader_rsp-----------%d,%ld", rq->rspHeader->scode,rq->rspHeader->contentLength);
    flog(log);
}



ResponseQueue* create_rsp(){
    ResponseQueue* rsp = (ResponseQueue*)malloc(sizeof(ResponseQueue));
    memset(rsp,0, sizeof(ResponseQueue));
    return rsp;
}
void destroy_rsp(ResponseQueue* rsp){
    if (rsp != NULL){
        if(rsp->strHeader != NULL){
            free(rsp->strHeader);
        }
        if(rsp->rspHeader != NULL){
            free(rsp->rspHeader);
        }
        free(rsp);
        rsp = NULL;
    }
}
static void parse_rsp_head(ResponseQueue* rq, const char *buf, int len){
    if (rq->strHeader != NULL){
        return;
    }
    char *tmpStr = (char *)buf;
    char *substr = "\r\n";
    while(tmpStr){
        char *s = strstr(tmpStr, substr);
        if (s != NULL){
            tmpStr = s+4;
        }else{
            break;
        }
    }

    int headLen = tmpStr - buf;
    if(headLen > 0){
        rq->strHeader = (char*)malloc(headLen);
        memset(rq->strHeader,0, headLen);
        memcpy(rq->strHeader, buf, headLen);
        flog(rq->strHeader);
        rq->_state = http_content;
        getheader(rq);
    }

    char log[256];
    sprintf(log, "--------parse_rsp_head----_state--------,%d", rq->_state);
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