#pragma once
#include "../../L2/buffer/stream.h"
#ifndef _WIN32
#ifndef SOCKET
#define SOCKET int
#endif
#else
#ifndef SOCKET
#include <WinSock2.h>
#endif
#endif


namespace network_l3 {
    using namespace type_l0;

    class sock_obj {
    public:
        virtual void destroy() = 0;
        virtual const char *type() const = 0;
        //return "listen" or "connection"
        virtual void *inst() = 0;
        virtual void on_event(unsigned int Event) {};
    };

    class connection :public sock_obj{
    public:
        virtual const char *state(const char *state_type) = 0;
        //"conn"  => return: "connecting", "fine", "breaked", "closed"; 
        //"read"  => "empty", "ready"; 
        //"write" => "ready", "full";
        //"error" => "breaked", "fine";
        virtual dword_t write(const char *data, dword_t len) = 0;
        virtual dword_t read(char *buf, dword_t len) = 0;
        virtual dword_t write(container_l2::stream_wrapper &send_buf) = 0;
        virtual dword_t read(container_l2::stream_wrapper &recv_buf) = 0;
        virtual dword_t ctrl(const char *req, char *ack) = 0; //error code
        virtual void change_state() = 0;
        virtual SOCKET unbind() = 0; //socket 解除绑定
    };
    class listenner :public sock_obj {
    public:
        virtual connection *accept() = 0;
    };
    class tcp_sock_poll {
    public:
        virtual void destroy() = 0;
        virtual connection *connect(const char *peer_addr, const char *local_addr, dword_t time_out_msec) = 0;
        virtual connection *connect(SOCKET sock_fd) = 0;//绑定一个现有socket
        virtual listenner *listen(const char *local_addr) = 0;
        virtual dword_t query(sock_obj **obj_list, dword_t list_count) = 0;
        static tcp_sock_poll *create(allocator_l0::allocator *Allocator);
    };
}
#ifdef _WIN32
#include "sock_iocp.h"
#else
#include "sock_epoll.h"
#endif
