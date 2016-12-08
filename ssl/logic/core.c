//
// Created by Administrator on 2016/12/6.
//

#include "core.h"
#include "log.h"
#include "RspQueue.h"
#include "ReqQueue.h"
#include "gettime.h"
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
            head->_ref_count=0;
            head->curr_time = 0;
            head->connect_end=0;
            head->start_time=0;
            head->connect_start=0;
            head->reqQueue = create_req();
            head->rspQueue = create_rsp();
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
    push_req(ci->reqQueue,buf, len, getSysTime());
}

static void on_down(ConnectionInfo* ci, const char *buf, int len){
    flog("on_dowm");
    if ( ! is_req_http(ci->reqQueue) ) {
        ci->rspQueue->_downsize += len;
        return;
    }
    int size = 0;
    while ( len ) {
        flog("push_rsp");
        size = push_rsp(ci->rspQueue,buf, len, getSysTime());
        buf += size;
        len -= size;
        if ( ci->rspQueue->response != NULL && strlen(ci->rspQueue->response) > 0 ) {

            switch ( ci->rspQueue->_state )
            {
                case http_head:
                    report(ci);
                    break;
                case protocol_error:
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
}



static void report(ConnectionInfo* ci){
    flog("report");
//    container_l2::smart_buffer<char, 1024 * 10> buffer( (_response.response.size() + _requests._reqs.front().request.size() + 1024 ) * 2 );
//
//    protocol3_http::response_decoder & res_decoder = _response._res;
//    RequestInfo &req = _requests._reqs.front();
//    UINT64 ret_size = 0;
//    protocol3_http::request_decoder req_decoder(get_allocator());
//    {
//        container_l2::stream<1000> req_stream;
//        req_stream.push_back(req.request.c_str(), req.request.size());
//        container_l2::my_stream_wrapper<1000> wrapper(req_stream);
//        req_decoder.parse(wrapper);
//
//        ret_size = req.recved;
//
//        if ( res_decoder.trans_mode() == protocol3_http::response_decoder::byclose )
//        {
//            ret_size = _response._downsize - _response.response.size();
//        }
//        else if ( res_decoder.trans_mode() != protocol3_http::response_decoder::chunked )
//        {
//            ret_size = res_decoder.ret_size() - _response._content_left;
//        }
//    }
//    container_l2::string url(_use_ssl?"https://":"http://", get_allocator());
//    url.append(protocol3_http::get_val(req_decoder.params(), "Host"));
//    //����uri�д�http ����https���������
//    const char *str = req_decoder.query("uri");
//    char *str1 = strstr(str, "http://");
//    char *str2 = strstr(str, "https://");
//    if (str1 || str2) {
//        url.clear();
//    }
//    url.append(str);
//
//    int size = sprintf(buffer.buffer(), "REQ|%d:%s|%lld,%lld,%lld,%lld|%d|%d|"
//            , 0
//            , "success"
//            , req.start_time / 1000
//            , req.req_end_time / 1000
//            , req.response_time / 1000
//            , this->curr_time / 1000
//            , fd
//            , res_decoder.rcode()
//    );
//    int base64ptr = size;
//    size += base64_encode((unsigned const char *)req.request.c_str(), req.request.size(), buffer.buffer() + size);
//    size += sprintf(buffer.buffer() + size, "|");
//    base64ptr = size;
//    size += base64_encode((unsigned const char *)_response.response.c_str(), _response.response.size(), buffer.buffer() + size);
//    size += sprintf(buffer.buffer() + size, "|%lld|%lld|%s", req.upload + req.request.size(), ret_size + _response.response.size(), url.c_str());
//    _instance->on_task(buffer.buffer());
//    _requests._reqs.pop_front();
    remove_conn(ci);
}

void on_user_close(ConnectionInfo* ci, int result_code){
    flog("on_user_close");
//    if ( ! _requests.is_http() ) return;
//    if ( _requests._reqs.size() == 0 ) return ;
//
//    while ( _requests._reqs.size() ) {
//
//        container_l2::smart_buffer<char, 1024 * 20> buffer(_requests._reqs.front().request.size() * 2 + _response.response.size() * 2 + 1024 * 2);
//
//        RequestInfo &req = _requests._reqs.front();
//        protocol3_http::request_decoder req_decoder(get_allocator());
//        {
//            container_l2::stream<1000> req_stream;
//            req_stream.push_back(req.request.c_str(), req.request.size());
//            container_l2::my_stream_wrapper<1000> wrapper(req_stream);
//            req_decoder.parse(wrapper);
//        }
//        // https or http process
//        container_l2::string url("");
//        if ( _use_ssl || _ssl )
//        {
//            url.append("https://");
//        }
//        else
//        {
//            url.append("http://");
//        }
//
//        url.append(protocol3_http::get_val(req_decoder.params(), "Host"));
//        //����uri�д�http ����https���������
//        const char *str = req_decoder.query("uri");
//        char *str1 = strstr(str, "http://");
//        char *str2 = strstr(str, "https://");
//        if (str1 || str2) {
//            url.clear();
//        }
//        url.append(str);
//
//        const char *error_desc = "success";
//        switch ( result_code )
//        {
//            case -1: error_desc = "Closed By Local";
//                break;
//            case -2: error_desc = "Closed By Peer";
//                break;
//            case -3: error_desc = strerror(_err_num);
//                break;
//            case -4: error_desc = "chunk parse error";
//                break;
//            default:
//                error_desc = strerror(_err_num);
//                break;
//        }
//        protocol3_http::response_decoder & res_decoder = _response._res;
//        if ( res_decoder.parsed() && res_decoder.trans_mode() == protocol3_http::response_decoder::byclose && (result_code == -2 || result_code == -1) ) {
//            result_code = 0; error_desc = "success";
//        }
//        int size = sprintf(buffer.buffer(), "REQ|%d:%s|%lld,%lld,%lld,%lld|%d|"
//                , result_code
//                , error_desc
//                , req.start_time / 1000
//                , req.req_end_time / 1000
//                , req.response_time / 1000
//                , this->curr_time / 1000
//                , fd
//        );
//        if ( res_decoder.parsed() ) {
//            size += sprintf(buffer.buffer() + size, "%d", res_decoder.rcode());
//        }
//        size += sprintf(buffer.buffer() + size, "|");
//        size += base64_encode((unsigned const char *)req.request.c_str(), req.request.size(), buffer.buffer() + size);
//        size += sprintf(buffer.buffer() + size, "|");
//        size += base64_encode((unsigned const char *)_response.response.c_str(), _response.response.size(), buffer.buffer() + size);
//        size += sprintf(buffer.buffer() + size, "|%lld|%lld|%s", req.upload + req.request.size(), _response._downsize, url.c_str());
//
//        _instance->on_task(buffer.buffer());
//        _requests._reqs.pop_front();
//    }
    remove_conn(ci);
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
        sprintf(  &buf
                , "SSL|%d:%s|%lld,%lld|%d"
                , err_code, err_code?"fail":"success"
                , conn_info->connect_start / 1000
                , (conn_info->connect_end?conn_info->connect_end:conn_info->curr_time) / 1000
                , (int)conn_info->_ssl);
        flog(buf);
    }
}

void on_read_end(ConnectionInfo *ci, char *buf, UINT64 start_time, int ret){
    flog("----on_read_end----");
    flog(buf);
    if ( ci->connect_end == 0 ) {
        ci->connect_end = start_time;
        on_connect_finished(ci,0);
    }
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


void on_write_end(ConnectionInfo *ci, char *buf, int len, UINT64 start_time, int ret){
    flog("----on_write_end----");
    flog(buf);
    if ( ci->connect_end == 0 ) {
        ci->connect_end = start_time;
        on_connect_finished(ci,0);
    }
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