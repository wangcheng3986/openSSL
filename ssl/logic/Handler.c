//
// Created by Administrator on 2016/12/6.
//

#include <stddef.h>
#include "Handler.h"
#include "core.h"
#include "entity.h"
#include "log.h"

void handle_ssl_new(SSL_NEW* data){
    if(data != NULL){
        flog("handle_ssl_new");
        ConnectionInfo* ci = get(data->_ret);
        ci->_ssl = data->_ret;
        ci->start_time = data->_begin_time;
    }
}
void handle_ssl_free(SSL_FREE* data){
    if(data != NULL){
        flog("handle_ssl_free");
        ConnectionInfo* ci = get(data->_ssl);
        if (ci->start_time <= data->_begin_time){
            ci->curr_time = data->_begin_time;
        }
        on_user_close(ci,-1);
    }

}
void handle_ssl_connect(SSL_CONNECT* data){
    if(data != NULL){
        flog("handle_ssl_connect");
        ConnectionInfo* ci = get(data->_ssl);
        ci->curr_time = data->_end_time;

        if ( ci->connect_start == 0 )
            ci->connect_start = data->_begin_time;
        if ( data->_ret > 0 ) {
            ci->connect_end = data->_end_time;
            on_connect_finished(ci, 0);
        }
    }

}
void handle_ssl_read(SSL_READ* data){
    if(data != NULL){
        flog("handle_ssl_read");
        ConnectionInfo* ci = get(data->_ssl);
        ci->curr_time = data->_end_time;
        on_read_end(ci, (char*)data->_buf, data->_begin_time, data->_ret);
    }
}
void handle_ssl_write(SSL_WRITE* data){
    if(data != NULL){
        flog("handle_ssl_write");
        ConnectionInfo* ci = get(data->_ssl);
        ci->curr_time = data->_end_time;
        on_write_end(ci, (char*)data->_buf, data->_len, data->_begin_time, data->_ret);
    }
}