#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../util/vstl/L0/abstract/type.h"
namespace ssl_hook {
    typedef int (*FN_SSL_connect)(void *ssl);
    typedef int (*FN_SSL_get_fd)(void *ssl);
    class ssl_handler {
    public:
        virtual void on_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time) = 0;
        virtual void on_ssl_close(void *ssl, UINT64 start_time) = 0;
        virtual void on_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd) = 0;
        virtual void on_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time) = 0;
        virtual void on_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time) = 0;
        virtual void on_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time) = 0;
    };

}
namespace hook_core {
    class core_ctrl {
    public:
        virtual void destroy() = 0;
        virtual const char *ctrl(const char *cmd, const char *first, const char *second) = 0;
    };
}
ssl_hook::ssl_handler *get_ssl_handler();
