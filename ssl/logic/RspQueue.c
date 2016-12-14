//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "RspQueue.h"
#include <string.h>
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

static void getheader(ResponseQueue* rq){
    char dst[10][80];
    int cnt = split(dst, rq->strHeader, "\r\n");

    char *s = strstr(dst[0], "HTTP/1.1");
    if(s != NULL){
        char list[5][80];
        int ll = split(list, dst[0], " ");
        if(ll > 2){
            rq->scode = atoi(list[1]);
        }
    }
//    int i = 1;
//    for (; i < cnt; i++)
//    {
//        flog(dst[i]);
//        char *s = strstr(dst[i], "Content-Length");
//        if(s != NULL){
//            char list[3][80];
//            int ll = split(list, dst[i], " ");
//            if(ll == 2){
//                rq->rspHeader->contentLength = atol(list[1]);
//                break;
//            }
//        }
//    }
    char log[80];
    sprintf(log, "--------getheader_rsp-----------%d,%ld", rq->scode,rq->contentLength);
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
        free(rsp);
        rsp = NULL;
    }
}
static void parse_rsp_head(ResponseQueue* rq, const char *buf){
    if (rq->strHeader != NULL || buf == NULL){
        return;
    }

    rq->strHeader = (char*)malloc(strlen(buf)+1);
    memset(rq->strHeader,0, strlen(buf)+1);
    strcpy(rq->strHeader, buf);
    flog(rq->strHeader);
    rq->_state = http_content;
    getheader(rq);
}

void push_rsp(ResponseQueue* rq, const char *buf, int len){
    flog("push_rsp");
    if(rq!= NULL){
        switch ( rq->_state )
        {
            case http_head:
                parse_rsp_head(rq, buf);
                break;
            case http_content:{
                flog("push_rsp--http_content");
//                if(rq->rspHeader!= NULL ){
//                    if(rq->rspHeader->contentLength == (rq->_downsize - strlen(rq->strHeader)) ){
//                        flog("http_end------------------");
//                        rq->_state = http_end;
//                    }
//
//                }
            }
                break;
            case http_end:
                break;
        }
        rq->_downsize += len;

        char log[256];
        sprintf(log, "--------push_rsp------------%ld,%d",rq->_downsize,len);
        flog(log);
    }
}