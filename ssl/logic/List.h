//
// Created by Administrator on 2016/12/7.
//

#ifndef LOGIC_LIST_H
#define LOGIC_LIST_H

#include "entity.h"
#include "../../../hook/jni/modules/plt_android/linuxtls.h"

List * list_create();
Node * list_front(List* l);
Node * list_back(List* l);
void list_push_back(List* l,Node *new_node);
void list_push_front(List* l,Node *new_node);
Node *list_pop_front(List* l) ;
Node *list_pop_back(List* l);
void list_erase(List* l,Node *pnode);
int list_size(List* l);

void list_destroy(List* l);
#endif //LOGIC_LIST_H
