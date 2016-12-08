//
// Created by Administrator on 2016/12/7.
//

#ifndef LOGIC_STREAM_H
#define LOGIC_STREAM_H

#include "entity.h"
Stream* stream_create();
void stream_clear(Stream* s);
dword_t stream_push_back(Stream* s, const char *data, dword_t len) ;
Node *stream_seek(Stream* s, dword_t offset, word_t* node_offset);
dword_t stream_peek(Stream* s,char *buf, dword_t len, dword_t offset);
dword_t stream_pop_back(Stream* s, dword_t len, char *buf);
#endif //LOGIC_STREAM_H
