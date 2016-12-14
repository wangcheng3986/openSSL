//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_HANDLER_H
#define LOGIC_HANDLER_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "entity.h"

void handle_ssl_new(SSL_NEW* data,long id);
void handle_ssl_free(SSL_FREE* data,long id);
void handle_ssl_connect(SSL_CONNECT* data,long id);
void handle_ssl_read(SSL_READ* data,long id);
void handle_ssl_write(SSL_WRITE* data,long id);

#ifdef  __cplusplus
}
#endif

#endif //LOGIC_HANDLER_H
