#pragma once
#include "../L0/abstract/type.h"
#include "../L3/io/sock_io.h"
class fconnection {
public:
    typedef type_l0::dword_t dword_t;
    virtual void destroy() = 0;
    virtual dword_t read(char *buf, dword_t len) = 0;
    virtual dword_t write(const char *data, dword_t len) = 0;
    virtual const char *state(const char *state_type) = 0;
    //"read"  => "empty", "ready"; 
    //"write" => "ready", "full";
    //"error" => "breaked", "fine";
};
class ifilter {
public:
    virtual void destroy() = 0;
    virtual bool can_release() const = 0;
    virtual void on_event(fconnection *conn) = 0;
    virtual void loop_check() = 0;
};
class helper {
public:
    virtual void *query_interface(const char *interface_name) = 0;
    virtual network_l3::tcp_sock_poll *create_socks() = 0;
};