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
            head->_connected= 0;
            head->_err_num=0;
            head->connect_end=0;
            head->connect_start=0;
            head->reqQueue = create_req();
            head->rspQueue = create_rsp(head->reqQueue);
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
    flog("on_up");
    push_req(ci->reqQueue,buf, len);
}

static void on_down(ConnectionInfo* ci, const char *buf, int len){
    char log[256];
    sprintf(log, "--------------------,%d", (int)ci->_ssl);
    flog(log);
    push_rsp(ci->rspQueue,buf, len);
    if ( ci->rspQueue->responseHeader != NULL && strlen(ci->rspQueue->responseHeader) > 0 ) {

        switch ( ci->rspQueue->_state )
        {
            case http_head:
                report(ci);
                break;
            case http_end:
                on_user_close(ci, -4);
                return;
                break;
            case http_content:
                break;
            default:
                break;
        }
    }
}



static void report(ConnectionInfo* ci){
    flog("report");
    on_user_close(ci,0);
}

void on_user_close(ConnectionInfo* ci, int result_code){
    flog("on_user_close");
    if ( ci->reqQueue->requestHeader == 0 ) return ;

    int total = strlen(ci->reqQueue->requestHeader) + strlen(ci->rspQueue->responseHeader)+1024;
    char* report = (char*)malloc(total);
    ///URL
    char* url = "https://";
    char* host = "host";
    char* uri = "uri";
    char* tmp = strstr(uri, host);
    if (tmp != uri){
        strcat(url,host);
    }
    strcat(url,uri);
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
    flog("on_user_close2");
    int statuscode = 200;
    char * reqHead = (char *)malloc(strlen(ci->reqQueue->requestHeader)+1);
    memset(reqHead,0,strlen(ci->reqQueue->requestHeader)+1);
    base64_encode((const unsigned char *)ci->reqQueue->requestHeader, reqHead, strlen(ci->reqQueue->requestHeader));

    char * rspHead = (char *)malloc(strlen(ci->rspQueue->responseHeader)+1);
    memset(rspHead,0,strlen(ci->rspQueue->responseHeader)+1);
    base64_encode((const unsigned char *)ci->rspQueue->responseHeader, rspHead, strlen(ci->rspQueue->responseHeader));
    /**
        * 结果|req开始时间,req结束时间,第一次收到response时间,最后一次收到response时间|socket id|状态码|request
        * headers|response headers|send字节数|recv字节数|URL
        */
    flog("on_user_close3");
    sprintf(report, "REQ|%d:%s|%lld,%lld,%lld,%lld|%d|%d|%s|%s|%lld|%lld|%s"
            , result_code
            , error_desc
            , ci->reqQueue->req_start_time
            , ci->reqQueue->req_end_time
            , ci->rspQueue->rsp_start_time
            , ci->rspQueue->rsp_end_time
            , (int)ci->_ssl
            , statuscode
            ,reqHead
            ,rspHead
            ,ci->reqQueue->_upsize
            ,ci->rspQueue->_downsize
            ,url
    );
    flog(report);
    remove_conn(ci);
    flog("on_user_close--end");
}

void on_connect_finished(ConnectionInfo *conn_info, int err_code){
    flog("----on_connect_finished----");
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
        flog(buf);
    }
}

void on_read_end(ConnectionInfo *ci, char *buf, int ret){
    flog("----on_read_end----");


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
    flog("----on_write_end----");

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