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

static void getheader(ResponseQueue* rq){
    char *result = NULL;
    char* tmpHeader = (char*)malloc(sizeof(rq->strHeader));
    memcpy(tmpHeader,rq->strHeader,sizeof(rq->strHeader));
    result = strtok(tmpHeader, "\r\n");
    char* status = NULL;
    char* length = NULL;
    while( result != NULL )
    {
        if(strstr(result, "HTTP/1.1")){
            status = result;
        }else if(strstr(result, "Content-Length")){
            length = result;
        }
        if(status && length){
            break;
        }
        result = strtok(NULL, "\r\n");
    }

    if(status){
        result = strtok(status, " ");
        int index = 0;
        while( result != NULL )
        {
            index++;
            if(index == 2){
                rq->scode = atoi(result);
                break;
            }
            result = strtok(NULL, " ");
        }
    }
    if(length){
        result = strtok(length, " ");
        int index = 0;
        while( result != NULL )
        {
            index++;
            if(index == 2){
                rq->contentLength = atol(result);
                break;
            }
            result = strtok(NULL, " ");
        }
    }

    char log[30];
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
        flog(rq->strHeader);
        rq->_state = http_content;
        getheader(rq);
        flog(rq->strHeader);
    }
}

void push_rsp(ResponseQueue* rq, const char *buf, int len){
    if(rq!= NULL){
        switch ( rq->_state )
        {
            case http_head:
                parse_rsp_head(rq, buf);
                rq->left = rq->contentLength;
                break;
            case http_content:{
                flog("push_rsp--http_content");
                if(rq->left > 0){
                    rq->left -= len;
                    if(rq->left <= 0 ){
                        rq->_state = http_end;
                    }
                }
            }
                break;
            case http_end:
                break;
        }
        rq->_downsize += len;

        char log[256];
        sprintf(log, "--------push_rsp------------%ld,%ld,%d",rq->contentLength,rq->_downsize,rq->left);
        flog(log);
    }
}