//
// Created by Administrator on 2016/12/6.
//

#ifndef LOGIC_HANDLER_H
#define LOGIC_HANDLER_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "entity.h"

void handle_ssl_new(SSL_NEW* data);
void handle_ssl_free(SSL_FREE* data);
void handle_ssl_connect(SSL_CONNECT* data);
void handle_ssl_read(SSL_READ* data);
void handle_ssl_write(SSL_WRITE* data);

#ifdef  __cplusplus
}
#endif

#endif //LOGIC_HANDLER_H
