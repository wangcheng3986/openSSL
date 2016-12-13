//
// Created by Administrator on 2016/12/6.
//

#include <stddef.h>
#include "ReqQueue.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

static void parse_head(RequestQueue* rq,const char *buf, int len);

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
    flog("getheader--0000000000");
    if(rq->reqHeader == NULL){
        rq->reqHeader = (ReqHeader*)malloc(sizeof(ReqHeader));
        memset(rq->reqHeader, 0, sizeof(ReqHeader));
    }
    char dst[10][80];
    int cnt = split(dst, rq->strHeader, "\r\n");
    flog("getheader--11111111111");
    int i = 0;
    for (; i < cnt; i++)
    {
        flog(dst[i]);
        char *s = strstr(dst[i], "GET");
        if(s != NULL){
            flog("getheader--222222222");
            rq->reqHeader->protocol = 0;
            char list[3][80];
            int ll = split(list, dst[i], " ");
            if(ll == 3){
                rq->reqHeader->pa = (char*)malloc(sizeof(char)* strlen(list[1]));
                strcpy(rq->reqHeader->pa, list[1]);
                flog(rq->reqHeader->pa);
            }
        }else{
            flog("getheader--33333333333333");
            s = strstr(dst[i], "POST");
            if(s != NULL){
                rq->reqHeader->protocol = 1;
                char list[3][80];
                int ll = split(list, dst[i], " ");
                if(ll == 3){
                    rq->reqHeader->pa = (char*)malloc(sizeof(char)* strlen(list[1]));
                    strcpy(rq->reqHeader->pa, list[1]);
                    flog(rq->reqHeader->pa);
                }
            }
        }
        flog("getheader--44444444444");
        s = strstr(dst[i], "Host");
        if(s != NULL){
            char list[2][80];
            int ll = split(list, dst[i], " ");
            if(ll == 2){
                rq->reqHeader->host = (char*)malloc(sizeof(char)* strlen(list[1]));
                strcpy(rq->reqHeader->host, list[1]);
                flog(rq->reqHeader->host);
            }
        }
    }
    flog("getheader--99999999999");
}


static void parse_head(RequestQueue* rq,const char *buf, int len){
    if (rq->strHeader != NULL){
        return;
    }
    char *tmpStr = (char*)buf;
    char *substr = "\r\n";
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
        rq->strHeader = (char*)malloc(index);
        memset(rq->strHeader,0, index);
        memcpy(rq->strHeader, buf, index);
        flog(rq->strHeader);
        rq->_state = http_content;
        getheader(rq->strHeader);
    }

    char log[256];
    sprintf(log, "--------parse_REQ_head----_state--------%d", rq->_state);
    flog(log);
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
            flog("push_req:Protocol_head");
            parse_head(rq, buf, len);
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