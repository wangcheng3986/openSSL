//
// Created by Administrator on 2016/12/7.
//

#include <stddef.h>
#include "List.h"
#include "Node.h"
List * list_create(){
    List* l = (List*)malloc(sizeof(List));
    memset(l,0, sizeof(List));
    return l;
}

void list_destroy(List* l){
    if(l!= NULL){
        free(l);
        l= NULL;
    }
}


Node *list_front(List* l){
    return l->_left;
}
Node *list_back(List* l) {
    return l->_right;
}
void list_push_back(List* l,Node *new_node) {
    if ( l->_right == 0 ) list_push_front(l, new_node);
    else {
        node_insert_back(new_node,l->_right);
        l->_right = new_node;
        l->_count++;
    }
}

void list_push_front(List* l,Node *new_node) {
    node_insert_back(new_node, l->_left);
    if ( l->_right == 0 ) l->_right = new_node;
    l->_count++;
}

Node *list_pop_front(List* l) {
    if ( l->_left == 0 ) return 0;
    Node *ret = l->_left;
    node_remove(l->_left);
    l->_count--;
    if ( l->_count == 0 ) l->_right = 0;
    return ret;
}
Node *list_pop_back(List* l){
    if ( l->_right == 0 ) return 0;
    Node *ret = l->_right;
    node_remove(l->_right);
    l->_count--;
    l->_right = l->_count ? ret->_pre: 0;
    return ret;
}

void list_erase(List* l,Node *pnode){
    if(pnode != NULL && list_size(l)>0){
        node_remove(pnode);
        l->_count--;
    }
}
int list_size(List* l){
    return l->_count;
}
