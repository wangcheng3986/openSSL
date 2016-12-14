//
// Created by Administrator on 2016/12/6.
//

#include <stddef.h>
#include "Handler.h"
#include "core.h"
#include "entity.h"
#include "log.h"

void handle_ssl_new(SSL_NEW* data,long id){
    if(data != NULL){
        ConnectionInfo* ci = get(data->_ret);
        ci->_ssl = data->_ret;
    }
}
void handle_ssl_free(SSL_FREE* data,long id){
    if(data != NULL){
        ConnectionInfo* ci = get(data->_ssl);
        on_connect_finished(ci,-1);
        on_user_close(ci,-1);
    }
}
void handle_ssl_connect(SSL_CONNECT* data,long id){
    if(data != NULL){
        ConnectionInfo* ci = get(data->_ssl);
        if ( ci->connect_start == 0 )
            ci->connect_start = data->_begin_time;
        if ( data->_ret > 0 ) {
            ci->connect_end = data->_end_time;
            on_connect_finished(ci, 0);
        }
    }
}
void handle_ssl_read(SSL_READ* data,long id){
    if(data != NULL){
        ConnectionInfo* ci = get(data->_ssl);
        if (ci->rspQueue->rsp_start_time == 0){
            ci->rspQueue->rsp_start_time = data->_begin_time;
        }
        if (data->_ret>0 && ci->rspQueue->rsp_end_time < data->_end_time){
            ci->rspQueue->rsp_end_time = data->_end_time;
        }
        on_read_end(ci, (char*)data->_buf, data->_ret);
    }
}
void handle_ssl_write(SSL_WRITE* data,long id){
    if(data != NULL){
        ConnectionInfo* ci = get(data->_ssl);
        if (ci->reqQueue->req_start_time == 0){
            ci->reqQueue->req_start_time = data->_begin_time;
        }
        if (data->_ret>0 && ci->reqQueue->req_end_time < data->_end_time){
            ci->reqQueue->req_end_time = data->_end_time;
        }
        on_write_end(ci, (char*)data->_buf, data->_len, data->_ret);
    }
}