//
// Created by Administrator on 2016/12/7.
//

#ifndef LOGIC_NODE_H
#define LOGIC_NODE_H

#include "entity.h"


void node_insert_back(Node *n,Node *pre);
void node_remove(Node *n);
Node *node_next(Node* n);
Node *node_pre(Node* n);
void node_destroy(Node* n);
Node* node_create();

word_t node_append(Node* n, const char *data, word_t len);
word_t node_push_back(Node* n,const char *data, word_t len);
word_t node_push_front(Node* n,const char *data, word_t len) ;
word_t node_pop_front(Node* n, char *buf, word_t len );
word_t node_pop_back(Node* n, char *buf, word_t len ) ;
word_t node_replace(Node* n,const char *data, int len, word_t offset);
word_t node_peek(Node* n,char *buf, int len, word_t offset);
word_t node_total_size(Node* n);



#endif //LOGIC_NODE_H
