//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_REQQUEUE_H
#define LOGIC_REQQUEUE_H

#include "entity.h"

RequestQueue* create_req();
void push_req(RequestQueue* rq, const char *buf, int len, UINT64 curr_time);
int is_req_http(RequestQueue* rq);

#endif //LOGIC_REQQUEUE_H
