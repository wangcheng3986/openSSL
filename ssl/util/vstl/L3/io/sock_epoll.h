#pragma once
#include "../../L0/abstract/type.h"
#include "../../L0/abstract/unistd.h"
#include "../../L0/containers/rbtree.h"
#include "../../L0/abstract/allocator.h"
#include "../../L2/buffer/stream.h"
#include "win_epoll.h"
#pragma warning (disable:4311 4312 4200 4996)
namespace network_l3 {
using namespace unistd_l0;
class epoll_sock_poll :public tcp_sock_poll, public allocator_l0::contain_base {

        AutoNetwork _auto;

        struct ConnInfo {

            SOCKET _s;
            epoll_event _event;
            ConnInfo():_s(-1){}
        };
        class C_listenner :public listenner, public ConnInfo {

            epoll_sock_poll *_sock_poll;
            virtual void *inst() {
                ConnInfo *instance = this;
                return instance;
            }
            virtual void destroy() {
                _sock_poll->destroy(this);
            }
            virtual const char *type() const { return "listen"; }
            virtual connection *accept() {
                if ( _s != -1 ) {
                    SOCKADDR_IN acceptAddr = {0};
                    socklen_t acceptAddrLen = sizeof(acceptAddr);
                    SOCKET s = ::accept(_s, (sockaddr*)&acceptAddr, &acceptAddrLen);
                    if ( s == -1 ) return 0;
                    connection *conn = _sock_poll->connect(s);
                    if ( conn == 0 ) closesocket(s);
                    return conn;
                }
                return 0;
            }
        public:
            C_listenner(epoll_sock_poll *sock_poll, SOCKET s):_sock_poll(sock_poll){
                _s = s;
                sock_obj *sockobj = this;
                _event.data.ptr = sockobj;
                _event.events = EPOLLIN | EPOLLET;
                _sock_poll->bind(this);
            }

            ~C_listenner() {
                SOCKET s = _sock_poll->unbind(this);
                if ( s != -1 ) closesocket(s);
            }
        };
        class C_connection :public connection, public ConnInfo {
            epoll_sock_poll *_sock_poll;
            virtual void on_event(unsigned int Event) { 
                if ( _state == 'i' ) _state = (Event & EPOLLOUT) ?'f': 'b';
                else if ( _state == 'f' && (Event & EPOLLERR || Event & EPOLLHUP) ) _state = 'b';
            };
        public:
            DWORD _failed_time;
            char _state;
            char _read_state;
            char _write_state;
            int _errcode;
            virtual void destroy() {
                _sock_poll->destroy(this);
            }
            virtual void *inst() {
                ConnInfo *instance = this;
                return instance;
            }
            virtual void change_state() { if (_state == 'i') _state = 'f'; }

            virtual const char *state(const char *query_type) {

                switch ( *query_type ) {
                case 'c': //"conn"
                    switch ( _state ) {
                case 'f':
                    return "fine";
                case 'i':
                    return "in connecting";
                case 'b':
                    return "breaked";
                case 'c':
                    return "closed";
                default:
                    return "undefined";
                    }
                case 'r': //"read"
                    return (_event.events & EPOLLIN) ? "ready": "empty";
                case 'w': //"write"
                    return (_event.events & EPOLLOUT) ? "ready" : "full";
                case 'e': //"error"
                    if ( _state == 'b' ) return "breaked";
                    return ((_event.events & EPOLLERR) || _event.events & EPOLLHUP) ? "breaked" : "fine";

                }
                return 0;
            }
            virtual dword_t ctrl(const char *req, char *ack) {
                //error code;
                if ( *req == 'e' ) return _errcode;
                return 0;
            }


            virtual const char *type() const { return "connection"; }

            virtual dword_t write(const char *data, dword_t len) {

                if ( len == 0 ) return 0;
                dword_t send_len = ::send(_s, data, len, 0);
                if ( send_len < 0 ) {

                    int err_code = error_num();
                    if ( (err_code == EAGAIN) || (err_code == EWOULDBLOCK) ) {
                        _event.events &= ~EPOLLOUT;
                    }
                    else {
                        _event.events |= EPOLLERR;
                        _event.events = 0;
                        _errcode = err_code;
                        _state = 'b';
                    }
                }
                return (send_len > 0) ? send_len: 0;
            }
            virtual dword_t read(char *buf, dword_t len) {

                dword_t ret = 0;
                int rcv_size = 0;
                bool recv_done = false;
                do {
                    rcv_size = recv(_s, buf + ret, len - ret, 0);
                    if ( rcv_size < 0 ) { 
                        int err_num = error_num();
                        if ( err_num != EAGAIN && err_num != EWOULDBLOCK ) {
                            _event.events |= EPOLLERR;
                            _errcode = err_num;
                            _state = 'b'; break;
                        }
                        else {
                            recv_done = true;
                            _event.events &= ~EPOLLIN;
                        }
                    }
                    else if ( rcv_size == 0 ) { _errcode = error_num(); _state = 'b'; break; }
                    else ret += rcv_size;
                } while ( (! recv_done) && ret < len);
                return ret;
            }
            virtual dword_t write(container_l2::stream_wrapper &send_buf) {

                dword_t ret = 0;
                while ( ret < send_buf.size() ) {

                    char buffer[1024 * 100];
                    dword_t peek_size = send_buf.peek(buffer, sizeof(buffer), ret);
                    dword_t top_send = write(buffer, peek_size);
                    ret += top_send;
                    if ( top_send < peek_size ) break;
                }
                return ret;
            }
            virtual dword_t read(container_l2::stream_wrapper &recv_buf) {
                dword_t ret = 0;
                dword_t rcv_size = 0;
                bool recv_done = false;
                char buffer[1024 * 100];
                do {
                    rcv_size = read(buffer, sizeof(buffer));
                    if ( rcv_size ) {
                        recv_buf.push_back(buffer, rcv_size);
                        ret += rcv_size;
                    }
                } while ( rcv_size == sizeof(buffer) );
                return ret;
            }
            virtual SOCKET unbind() {
                return _sock_poll->unbind(this);
            }
        public:
            C_connection(epoll_sock_poll *sock_poll, SOCKET s) :_sock_poll(sock_poll), _state('f'),_write_state('n'), _read_state('e'),_errcode(0) {
                _s = s;
                sock_obj *sockobj = this;
                _event.data.ptr = sockobj;
                _event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLERR | EPOLLHUP; 
            }
            ~C_connection() {
                SOCKET s = _sock_poll->unbind(this);
                if ( s != -1 ) closesocket(s);
            }
        };
        typedef container_l0::rbtree<sock_obj *> RBTree;

        int _epfd;
    protected:
        friend class C_listenner;
        friend class C_connection;
        SOCKET unbind(ConnInfo *conn) {
            SOCKET ret = conn->_s;
            if ( ret != -1 ) {
                epoll_ctl(_epfd, EPOLL_CTL_DEL, conn->_s, &conn->_event);
                conn->_s = -1;
            }
            return ret;
        }
        bool bind(ConnInfo *conn) {
            sockblock(conn->_s, false);
            bool ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, conn->_s, &conn->_event) == 0;
            if ( ret ) conn->_event.events = 0;
            return ret;
        }
        void destroy(C_listenner *Listenner) {
            Listenner->~C_listenner();
            _allocator->deallocate(Listenner);
        }
        void destroy(C_connection *conn) {
            conn->~C_connection();
            _allocator->deallocate(conn);
        }
    public:
        epoll_sock_poll(allocator_l0::allocator *Allocator) :allocator_l0::contain_base(Allocator) {

            _epfd = epoll_create(10000);
            assert(_epfd != -1);
        }
        ~epoll_sock_poll() {
            (_epfd != -1)? epoll_release(_epfd): (void)0;
        }
        virtual void destroy() {
            allocator_l0::allocator *alloc = _allocator;
            this->~epoll_sock_poll();
            alloc->deallocate(this);
        }
        virtual connection *connect(const char *peer_addr, const char *local_addr, dword_t time_out_msec) {

            _SERVICE_ADDR peer;
            _SERVICE_ADDR local;
            peer.init(peer_addr, 0);
            local.init(local_addr, 0);
            SOCKET s = network_l3::async_connect(peer.ip, peer.port, local.ip, local.port);
            if ( s == INVALID_SOCKET ) return 0;
            connection *ret = connect(s);
            if ( ret ) {
                C_connection *conn = static_cast<C_connection *>(ret);
                conn->_state = 'i';
                conn->_failed_time = GetTickCount() + time_out_msec;
            }
            else closesocket(s);
            return ret;
        }
        virtual connection *connect(SOCKET sock_fd) {

            C_connection *conn = static_cast<C_connection *>(_allocator->allocate(sizeof(C_connection)));
            if ( conn ) {
                ::new ((void*)conn) C_connection(this, sock_fd);
                if ( ! bind(conn) ) {
                    conn->_s = -1;
                    conn->~C_connection();
                    _allocator->deallocate(conn);
                    conn = 0;
                }
            }
            return conn;
        }
        virtual listenner *listen(const char *local_addr) {

            _SERVICE_ADDR addr;
            addr.init(local_addr, 0);
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
            if ( sock == INVALID_SOCKET ) return 0;

            set_reuse(sock, true);
            sockblock(sock, false);

            SOCKADDR_IN listen_addr = {0};
            listen_addr.sin_family = AF_INET;
            listen_addr.sin_addr.s_addr = htonl(addr.ip);
            listen_addr.sin_port = htons(addr.port);

            if ( ::bind(sock, (sockaddr*)&listen_addr, sizeof(listen_addr)) == SOCKET_ERROR ) {

                printf("bind error %s\n", strerror(errno));
                closesocket(sock); return 0;
            }

            if ( ::listen(sock, 32) == SOCKET_ERROR ) {

                printf("listen error %s\n", strerror(errno));
                closesocket(sock); return 0;
            }

            C_listenner *listen_obj = static_cast<C_listenner*>(_allocator->allocate(sizeof(C_listenner)));
            if ( listen_obj ) {
                new ((void*)listen_obj) C_listenner(this, sock);
            }
            else closesocket(sock);
            return listen_obj;
        }
        virtual dword_t query(sock_obj **obj_list, dword_t list_count) {
            epoll_event evt_list[1000];
            dword_t use_count = (dword_t)unistd_l0::Min<size_t>(list_count, sizeof(evt_list)/sizeof(evt_list[0]));
            int r_count = epoll_wait(_epfd, evt_list, use_count, 0);
            if ( r_count == -1 ) {
                printf("tcp_sock_poll(%d) %s\n", _epfd, strerror(error_num()));
                r_count = 0;
            }
            if ( r_count ) {
                for (int i = 0; i < r_count; i++ ) {
                    obj_list[i] = (sock_obj *)(evt_list[i].data.ptr);
                    ConnInfo *conn = (ConnInfo*)(obj_list[i]->inst());
                    conn->_event.events = evt_list[i].events;
                    obj_list[i]->on_event(conn->_event.events);
                }
            }
            return r_count;
        }
    };
    inline 
        tcp_sock_poll *tcp_sock_poll::create(allocator_l0::allocator *Allocator) {
            epoll_sock_poll *inst = static_cast<epoll_sock_poll *>(Allocator->allocate(sizeof(epoll_sock_poll)));
            new ((void*)inst) epoll_sock_poll(Allocator);
            return inst;
    }
}
