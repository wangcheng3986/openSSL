//
// Created by Administrator on 2016/12/7.
//

#ifndef LOGIC_RSPQUEUE_H
#define LOGIC_RSPQUEUE_H

#include "entity.h"

ResponseQueue* create_rsp(RequestQueue* rq);
void push_rsp(ResponseQueue* rq, const char *buf, int len);
void destroy_rsp(ResponseQueue* rsp);

#endif //LOGIC_RSPQUEUE_H
