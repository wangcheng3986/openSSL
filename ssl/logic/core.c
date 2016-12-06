//
// Created by Administrator on 2016/12/6.
//

#include "core.h"
#include "log.h"

static void on_up(ConnectionInfo* ci,const char *buf, int len);
static void on_down(ConnectionInfo* ci, const char *buf, int len);
static void report(ConnectionInfo* ci);

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

void on_up(ConnectionInfo* ci,const char *buf, int len){

}

void on_down(ConnectionInfo* ci, const char *buf, int len){

}

void report(ConnectionInfo* ci){

}

void on_user_close(ConnectionInfo* ci, int result_code){
    on_connect_finished( ci, -1);
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
                , conn_info->connect_start / 1000
                , (conn_info->connect_end?conn_info->connect_end:conn_info->curr_time) / 1000
                , conn_info->_ssl);
        flog(buf);
    }
}

void on_read_end(ConnectionInfo *ci, char *buf, UINT64 start_time, int ret){
//    if ( conn_info->connect_end == 0 ) {
//        conn_info->connect_end = start_time;
//        on_connect_finished(fd, conn_info);
//    }
//    int read_size = -1;
//    if ( ret > 0 )
//    {
//        read_size = ret;
//        conn_info->on_down(fd, buf, read_size);
//    }
//    else
//    {
//        //recv 0, �Է��ر�����
//        if ( ret == 0 )
//        {
//            on_user_close(fd, conn_info, -2);
//        }
//            //recv���󣬷��첽����
//        else if ( conn_info->_err_num != EAGAIN && conn_info->_err_num != EWOULDBLOCK )
//        {
//            on_break(fd, conn_info);
//        }
//    }
}


void on_write_end(ConnectionInfo *ci, char *buf, int len, UINT64 start_time, int ret){
//    if ( conn_info->connect_end == 0 ) {
//        conn_info->connect_end = start_time;
//        on_connect_finished(fd, conn_info);
//    }
//    int up_size = -1;
//    if ( ret > 0 )
//    {
//        up_size = ret;
//    }
//    else
//    {
//        if ( conn_info->_err_num == EAGAIN || conn_info->_err_num == EWOULDBLOCK)
//        {
//            up_size = len;
//        }
//        else //send ʧ��
//        {
//            on_break(fd, conn_info);
//        }
//    }
//    if ( up_size > 0 )
//        conn_info->on_up(buf, up_size);
}