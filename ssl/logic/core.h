//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_CORE_H
#define LOGIC_CORE_H

#include <stddef.h>
#include "entity.h"



ConnectionInfo* get(void* ssl);

void on_connect_finished(ConnectionInfo *conn_info, int err_code);
void on_user_close(ConnectionInfo* ci, int result_code);
void on_read_end(ConnectionInfo *ci, char *buf, int ret);
void on_write_end(ConnectionInfo *ci, char *buf, int len, int ret);






#endif //LOGIC_CORE_H
