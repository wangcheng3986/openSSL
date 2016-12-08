//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "stream.h"
#include "List.h"
#include "Node.h"


Stream* stream_create(){
    Stream* s = (Stream*)malloc(sizeof(Stream));
    memset(s,0, sizeof(Stream));
    s->_list = list_create();
    return s;
}

void stream_clear(Stream* s){
    if(s->_list!= NULL){
        while ( list_size(s->_list) > 0 ) {
            Node *top_node = list_pop_front(s->_list);
            node_destroy(top_node);
        }
    }
    s->_used = 0;
}

dword_t stream_push_back(Stream* s, const char *data, dword_t len) {
    dword_t ret = 0;
    while ( len ) {

        Node *top_node = list_back(s->_list);
        if ( top_node == 0 ) {

            top_node = (Node *)malloc(sizeof(Node));
            memset(top_node,o, sizeof(Node));
            list_push_back(s->_list,top_node);
        }
        word_t push_max = Min(node_size, len);
        word_t writed = node_push_back(top_node,data + ret, push_max);
        s->_used += writed;
        ret += writed;
        len -= writed;
    }
    return ret;
}

Node *stream_seek(Stream* s, dword_t offset, word_t* node_offset)  {

    if ( s->_used <= offset ) return 0;
    dword_t seeked = 0;
    Node *top_node = list_front(s->_list);
    while ( seeked + node_total_size(top_node) <= offset ) {
    seeked += node_total_size(top_node);
    top_node = node_next(top_node);
    }
    *node_offset = (word_t)(offset - seeked);
    return top_node;
}

dword_t stream_peek(Stream* s,char *buf, dword_t len, dword_t offset)  {
    word_t node_offset = 0;
    Node *top_node = stream_seek(s, offset, &node_offset);
    if ( top_node == 0 ) return 0;

    dword_t ret = Min (s->_used - offset, len);
    dword_t peeked = 0;
    while ( peeked < ret ) {
        word_t node_peeked = node_peek(top_node,buf + peeked,  Min(ret - peeked, node_total_size(top_node) - node_offset), node_offset);
        peeked += node_peeked;
        node_offset = 0;
        top_node = node_next(top_node);
    }
    return ret;
}

dword_t stream_pop_back(Stream* s, dword_t len, char *buf) {
    len = Min (s->_used, len);
    dword_t ret = len;
    while ( len ) {
        Node *top_node =list_back( s->_list));
        word_t poped = Min (len, node_total_size(top_node));
        node_pop_back(top_node,buf ?buf + len - poped: 0, poped);
        len -= poped;
        if ( node_total_size(top_node) == 0 ) {
            list_pop_back(s->_list);
            node_destroy(top_node);
    }
    }
    s->_used -= ret;
    return ret;
}