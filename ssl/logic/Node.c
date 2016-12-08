//
// Created by Administrator on 2016/12/7.
//

#include "Node.h"
static void read(Node* n, word_t begin, char *data, int len);
static void write(Node* n, word_t begin, const char *data, int len);

static void write(Node* n, word_t begin, const char *data, int len) {

    word_t copy_first = Min(len, node_size - begin);
    memcpy(n->_data + begin, data, copy_first);
    if ( copy_first < len ) {
        memcpy(n->_data, data + copy_first, len - copy_first);
    }
}
static void read(Node* n, word_t begin, char *data, int len) {

    word_t copy_first = Min(len, node_size - begin);
    memcpy(data, n->_data + begin, copy_first);
    if ( copy_first < len ) {
        memcpy(data + copy_first, n->_data, len - copy_first);
    }
}

Node *node_next(Node* n)  {
    return n->_next;
}
Node *node_pre(Node* n)  {
    return n->_pre;
}

void node_insert_back(Node *n,Node *pre) {
    n->_pre = pre;
    n->_next = pre->_next;
    pre->_next = n;
    if ( n->_next ) {
        n->_next->_pre = n;
    }
}

void node_remove(Node *n) {
    n->_pre->_next = n->_next;
    if ( n->_next ) {
        n->_next->_pre = n->_pre;
    }
}

Node* node_create(){
    Node* n = (Node*)malloc(sizeof(Node));
    memset(n,0, sizeof(Node));
    return n;
}

void node_destroy(Node* n){
    if(n){
        free(n);
        n= NULL;
    }
}

word_t node_append(Node* n, const char *data, word_t len) {
    return push_back(n,data, len);
}
word_t node_push_back(Node* n,const char *data, word_t len) {
    //copy size
    word_t ret = Min(len, node_size - n->_used);
    word_t begin = n->_start + n->_used;//copy begin
    int a =  begin >= node_size?node_size:0;
    begin -= a;
    write(n,begin, data, ret);
    n->_used += ret;
    return ret;
}
word_t node_push_front(Node* n,const char *data, word_t len) {
    word_t ret = Min(len, node_size - n->_used);//copy size
    if ( ret < len ) data += len - ret;
    n->_start = n->_start + node_size - ret;
    n->_start -= (n->_start >= node_size) ? node_size: 0;
    write(n, n->_start, data, ret);
    n->_used += ret;
    return ret;
}

word_t node_pop_front(Node* n, char *buf, word_t len ) {
    word_t ret = Min(len, n->_used);//copy size
    if ( buf ) read(n,n->_start, buf, ret);
    n->_start += ret;
    n->_start -= (n->_start >= node_size) ? node_size: 0;
    n->_used -= ret;
    return ret;
}

word_t node_pop_back(Node* n, char *buf, word_t len ){
    word_t ret = Min(len, n->_used);//copy size
    word_t begin = n->_start + n->_used - ret;
    begin -= (begin >= node_size) ? node_size: 0;
    if ( buf ) read(n, begin, buf, ret);
    n->_used -= ret;
    return ret;
}

word_t node_replace(Node* n,const char *data, int len, word_t offset) {
    if ( n->_used <= offset ) return 0;
    word_t ret = Min(len, n->_used - offset);//copy size
    word_t begin = n->_start + offset;
    begin -= (begin >= node_size) ? node_size: 0;
    write(n,begin, data, ret);
    return ret;
}

word_t node_peek(Node* n,char *buf, int len, word_t offset){
    if ( n->_used <= offset ) return 0;
    word_t ret = Min(len, n->_used - offset);//copy size
    word_t begin = n->_start + offset;
    begin -= (begin >= node_size) ? node_size: 0;
    read(n,begin, buf, ret);
    return ret;
}

word_t node_total_size(Node* n){
    return n->_used;
}
