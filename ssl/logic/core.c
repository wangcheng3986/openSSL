//
// Created by Administrator on 2016/12/6.
//

#include "core.h"
#include "log.h"
#include "RspQueue.h"
#include "ReqQueue.h"
#include "base64_encode.h"
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>

static ConnectionInfo* _SSLConnectionList = NULL;


static void on_up(ConnectionInfo* ci,const char *buf, int len);
static void on_down(ConnectionInfo* ci, const char *buf, int len);
static void report(ConnectionInfo* ci);
static void on_break(ConnectionInfo * ci);
static void remove_conn(ConnectionInfo * ci);


ConnectionInfo* get(void* ssl){
    ConnectionInfo* head = _SSLConnectionList;
    ConnectionInfo* tail = head;
    while (head){
        if(head->_ssl == ssl){
            return head;
        }
        head = head->next;
        if (head != NULL){
            tail = head;
        }
    }
    ConnectionInfo* ci = (ConnectionInfo*)malloc(sizeof(ConnectionInfo));
    memset(ci,0, sizeof(ConnectionInfo));
    ci->_ssl = ssl;
    ci->reqQueue = create_req();
    ci->rspQueue = create_rsp();
    ci->next = NULL;
    if(_SSLConnectionList == NULL){
        _SSLConnectionList = ci;
    }else{
        tail->next = ci;
    }
    return ci;
}

static void remove_conn(ConnectionInfo * ci){
    if(ci == NULL){
        return;
    }
    if(_SSLConnectionList == ci){
        _SSLConnectionList = NULL;
        destroy_req(ci->reqQueue);
        destroy_rsp(ci->rspQueue);
        free(ci);
        ci = NULL;
        return;
    }
    ConnectionInfo* cur = _SSLConnectionList->next;
    ConnectionInfo* pre = _SSLConnectionList;
    while (cur){
        if(cur->_ssl == ci->_ssl){
            pre->next = cur->next;
            break;
        }
        pre = cur;
        cur = cur->next;
    }

    destroy_req(ci->reqQueue);
    destroy_rsp(ci->rspQueue);
    free(ci);
    ci = NULL;
}


static void on_break(ConnectionInfo *ci)
{
    on_connect_finished(ci,-1);
    on_user_close(ci, -3);
}
static void on_up(ConnectionInfo* ci,const char *buf, int len){
    push_req(ci->reqQueue,buf, len);
}

static void on_down(ConnectionInfo* ci, const char *buf, int len){
    push_rsp(ci->rspQueue,buf, len);
    if ( ci->rspQueue->strHeader != NULL && strlen(ci->rspQueue->strHeader) > 0 ) {
        switch ( ci->rspQueue->_state )
        {
            case http_head:
                break;
            case http_end:
                report(ci);
                break;
            case http_content:
                break;
            default:
                break;
        }
    }
}



static void report(ConnectionInfo* ci){
    on_user_close(ci,0);
}

static char* strJoin(const char *s1, const char *s2)
{
    char *result = (char*)malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator3
    //in real code you would check for errors in malloc here
    if (result == NULL) return NULL;

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}

void on_user_close(ConnectionInfo* ci, int result_code){
    if ( ci->reqQueue->strHeader == 0 ) return ;
    if ( ci->reqQueue->reqHeader == 0 ) return ;
    if ( ci->reqQueue->reqHeader->pa == 0 ) return ;
    if ( ci->reqQueue->reqHeader->host == 0 ) return ;
    if ( ci->rspQueue->strHeader == 0 ) return ;
    int total = strlen(ci->reqQueue->strHeader) + strlen(ci->rspQueue->strHeader)+1024;
    char* report = (char*)malloc(total);
    ///URL
    char url[80];
    memset(url,0,80);
    strcat(url,"https://");
    //处理uri中带http 或者https的特殊情况
    const char *strHost2 = strJoin("https://",ci->reqQueue->reqHeader->host);
    int result = strncmp(ci->reqQueue->reqHeader->pa, ci->reqQueue->reqHeader->host, strlen(ci->reqQueue->reqHeader->host));
    //URI以host开头
    if ( result == 0) {
        strcat(url,ci->reqQueue->reqHeader->pa);
    }else{
        //URI以http+host开头
        result = strncmp(ci->reqQueue->reqHeader->pa, strHost2, strlen(strHost2));
        if ( result == 0) {
            memset(url,0,80);
            strcat(url,ci->reqQueue->reqHeader->pa);
        }else{//其他情况
            strcat(url,ci->reqQueue->reqHeader->host);
            strcat(url,ci->reqQueue->reqHeader->pa);
        }
    }

    //ERROR CODE
    const char *error_desc = "success";
    switch ( result_code )
    {
        case 0:
            break;
        case -1: error_desc = "Closed By Local";
            break;
        case -2: error_desc = "Closed By Peer";
            break;
        case -3: error_desc = strerror(ci->_err_num);
            break;
        case -4: error_desc = "chunk parse error";
            break;
        default:
            error_desc = strerror(ci->_err_num);
            break;
    }
    int len = 0;
    len = strlen(ci->reqQueue->strHeader)*2;
    char * reqHead = (char *)malloc(len);
    memset(reqHead,0,len);
    len = strlen(ci->rspQueue->strHeader)*2;
    char * rspHead = (char *)malloc(len);
    memset(rspHead,0,len);

    base64_encode((const unsigned char *)ci->reqQueue->strHeader, reqHead, strlen(ci->reqQueue->strHeader));
    base64_encode((const unsigned char *)ci->rspQueue->strHeader, rspHead, strlen(ci->rspQueue->strHeader));
    /**
        * 结果|req开始时间,req结束时间,第一次收到response时间,最后一次收到response时间|socket id|状态码|request
        * headers|response headers|send字节数|recv字节数|URL
        */

    sprintf(report, "REQ|%d:%s|%lld,%lld,%lld,%lld|%d|%d|%s|%s|%lld|%ld|%s"
            , result_code
            , error_desc
            , ci->reqQueue->req_start_time
            , ci->reqQueue->req_end_time
            , ci->rspQueue->rsp_start_time
            , ci->rspQueue->rsp_end_time
            , (int)ci->_ssl
            , ci->rspQueue->scode
            ,reqHead
            ,rspHead
            ,ci->reqQueue->_upsize
            ,ci->rspQueue->_downsize
            ,url
    );
    free(rspHead);
    free(reqHead);
    remove_conn(ci);

    fresult(report);
    if(ci->_callbackFunc){
        ci->_callbackFunc(report);
    }
}

void on_connect_finished(ConnectionInfo *conn_info, int err_code){
    if ( conn_info->_connected ) return;
    conn_info->_connected = 1;
    if ( conn_info->connect_start == 0 ) return;

    if ( conn_info->_ssl )
    {
        char buf[1024];
        memset(buf,0,1024);
        sprintf(  buf
                , "SSL|%d:%s|%lld,%lld|%d"
                , err_code, err_code?"fail":"success"
                , conn_info->connect_start
                , conn_info->connect_end
                , (int)conn_info->_ssl);
        fresult(buf);
        if(conn_info->_callbackFunc){
            conn_info->_callbackFunc(buf);
        }
    }
}

void on_read_end(ConnectionInfo *ci, char *buf, int ret){
    int read_size = -1;
    if ( ret > 0 )
    {
        read_size = ret;
        on_down(ci, buf, read_size);
    }
    else
    {
        if ( ret == 0 )
        {
            on_connect_finished(ci, -1);
            on_user_close(ci, -2);
        }
        else if ( ci->_err_num != EAGAIN && ci->_err_num != EWOULDBLOCK )
        {
            on_break(ci);
        }
    }
}


void on_write_end(ConnectionInfo *ci, char *buf, int len, int ret){
    int up_size = -1;
    if ( ret > 0 )
    {
        up_size = ret;
    }
    else
    {
        if ( ci->_err_num == EAGAIN || ci->_err_num == EWOULDBLOCK)
        {
            up_size = len;
        }
        else
        {
            on_break(ci);
        }
    }
    if ( up_size > 0 )
        on_up(ci, buf, up_size);
}