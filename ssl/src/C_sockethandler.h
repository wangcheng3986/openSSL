#pragma once

#ifdef  __cplusplus
extern "C" {
#endif


#include "../util/vstl/L0/containers/pool.h"
#include "sock_logic.h"
#include "hook_interface.h"
#include "linuxtls.h"
class C_sockethandler;
class param_object : public container_l0::pool_node {
protected:
    C_sockethandler *_handler;
    type_l0::dword_t _id;
    int _tid;
public:
    param_object() { _tid = gettid(); }

    virtual void destroy() = 0;

    virtual void exec() = 0;

    int tid() const { return _tid; }

    type_l0::dword_t id() const { return _id; }
};
typedef container_l2::map<qword_t, param_object *> TaskMap;

class C_sockethandler
        : public fd_atom, public hook_callback::logic_handler, public ssl_hook::ssl_handler {

    container_l0::pool<32> _pool;
    type_l0::dword_t _index;
    type_l0::dword_t _exec_index;
    allocator_l0::default_allocator _alloc;
    _SSLMap _sslconns;
    TaskMap _tasks;
    C_pthreadMutex _names_lock;
    protocol3_http::string_map _names_map;
    Tls _tls;

    void on_connect_finished(int fd, ConnectionInfo *conn_info, int err_code = 0);

    void on_break(int fd, ConnectionInfo *conn);

    void on_user_close(int fd, ConnectionInfo *conn, int result_code);

    void on_read_end(int fd, ConnectionInfo *conn, char *buf, UINT64 start_time, ssize_t ret);

    void on_write_end(int fd, ConnectionInfo *conn, char *buf, size_t len, UINT64 start_time,
                      ssize_t ret);

public:
    void run();

    type_l0::dword_t get_id();


    void hand_SSL_new(void *ctx, void *ret, UINT64 start_time, UINT64 end_time);

    void hand_SSL_free(void *ssl, UINT64 start_time);

    void hand_SSL_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd);

    void hand_SSL_read(void *ssl, void *buf, int num, int ret, UINT64 start_time, UINT64 end_time);

    void hand_SSL_write(void *ssl, const void *buf, int num, int ret, UINT64 start_time,
                        UINT64 end_time);

    void hand_SSL_set_fd(void *s, int fd, int ret, UINT64 start_time);


    virtual void on_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time);

    virtual void on_ssl_close(void *ssl, UINT64 start_time);

    virtual void on_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd);

    virtual void on_ssl_read(void *ssl, void *buf, int num, int ret, UINT64 start_time,
                             UINT64 end_time);

    virtual void on_ssl_write(void *ssl, const void *buf, int num, int ret, UINT64 start_time,
                              UINT64 end_time);

    virtual void on_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time);

    C_sockethandler(void);

    ~C_sockethandler(void);
};

#ifdef  __cplusplus
}
#endif