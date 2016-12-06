//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_CORE_H
#define LOGIC_CORE_H

#include <stddef.h>
#include "entity.h"

static ConnectionInfo* _SSLConnectionList = NULL;

ConnectionInfo* get(void* ssl);

void on_connect_finished(ConnectionInfo *conn_info, int err_code);
void on_read_end(ConnectionInfo *ci, char *buf, UINT64 start_time, int ret);
void on_write_end(ConnectionInfo *ci, char *buf, int len, UINT64 start_time, int ret);
void on_user_close(ConnectionInfo* ci, int result_code);





#endif //LOGIC_CORE_H
