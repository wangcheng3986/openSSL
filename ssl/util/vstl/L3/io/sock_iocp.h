#pragma once
#include "../../L0/abstract/unistd.h"
#include "../../L2/containers/map.h"
#include "win_epoll.h"
#pragma warning (disable:4311 4312 4200 4996)
#include "Mswsock.h"
#ifndef WSAID_CONNECTEX
typedef
BOOL
(PASCAL FAR * LPFN_CONNECTEX) (
                               IN SOCKET s,
                               IN const struct sockaddr FAR *name,
                               IN int namelen,
                               IN PVOID lpSendBuffer OPTIONAL,
                               IN DWORD dwSendDataLength,
                               OUT LPDWORD lpdwBytesSent,
                               IN LPOVERLAPPED lpOverlapped
                               );

#define WSAID_CONNECTEX \
{0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}


typedef
BOOL
(PASCAL FAR * LPFN_ACCEPTEX)(
                             IN SOCKET sListenSocket,
                             IN SOCKET sAcceptSocket,
                             IN PVOID lpOutputBuffer,
                             IN DWORD dwReceiveDataLength,
                             IN DWORD dwLocalAddressLength,
                             IN DWORD dwRemoteAddressLength,
                             OUT LPDWORD lpdwBytesReceived,
                             IN LPOVERLAPPED lpOverlapped
                             );

#define WSAID_ACCEPTEX \
{0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

#endif
static LPFN_CONNECTEX ConnectEx = 0;
static LPFN_ACCEPTEX _AcceptEx = 0;
namespace network_l3 {

    static bool in_connect( SOCKET s, SOCKET listen_sock = INVALID_SOCKET) {

        int seconds, bytes = sizeof(seconds);
        if ( listen_sock != INVALID_SOCKET ) setsockopt( s, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&listen_sock, sizeof(listen_sock) );
        if ( getsockopt( s, SOL_SOCKET, SO_CONNECT_TIME, (char *)&seconds, &bytes ) == SOCKET_ERROR ) return false;
        return seconds >= 0;
    }
    struct SocketAddress :public SOCKADDR_IN {

        SocketAddress(short family, u_short port, u_long ip) {
            sin_family = family;
            sin_port = port;
            sin_addr.s_addr = ip;
            sin_zero[0] = sin_zero[1] = sin_zero[2] = sin_zero[3] = sin_zero[4] = sin_zero[5] = sin_zero[6] = sin_zero[7] = 0;
        }
    };
    using namespace type_l0;

    struct task_obj : public OVERLAPPED {
        
        WSABUF _buffer; //任务数据
        dword_t _cp_size;//完成字节数
        sock_obj *_conn;//连接对象
        bool _failed;
        dword_t _error_code;
        task_obj() :_failed(false), _cp_size(0), _conn(0), _error_code(0){
            OVERLAPPED *p = this;
            memset(p, 0, sizeof(*p));
            _buffer.len = 0; _buffer.buf = 0;
        }
        virtual void destroy() = 0;
        virtual const char *type() const = 0;
    };

    static task_obj *query_task(HANDLE iocp) {

        DWORD cp_size = 0;
        void *context = 0;
        OVERLAPPED *request = 0;
        task_obj *task = 0;
        if ( ::GetQueuedCompletionStatus( iocp, (LPDWORD)&cp_size, (PULONG_PTR)&context, (LPOVERLAPPED*)&request, 0) ) {
            assert(request != 0);
            task = static_cast<task_obj *>(request);
        }
        else {
            DWORD error_code = WAIT_TIMEOUT;
            //see MSDN GetQueuedCompletionStatus
            if ( request ) {
                task = static_cast<task_obj *>(request);
                error_code = GetLastError();
                task->_error_code = error_code;
                if ( error_code != WAIT_TIMEOUT ) task->_failed = true;
                printf("errorcode = %d\n", error_code);
            }
            else error_code = WAIT_TIMEOUT;
        }
        if ( task ) task->_cp_size = cp_size;
        return task;
    }


    class iocp_sock_poll :public tcp_sock_poll, public allocator_l0::contain_base {
        AutoNetwork _auto;

        struct ConnInfo {

            SOCKET _s;
            char _requests;
            char _remoeved;
            ConnInfo():_s(INVALID_SOCKET), _requests(0), _remoeved(false){}
            ~ConnInfo() {
                if ( _s > -1 ) closesocket(_s);
            }
            void inc() { _requests++;}
            void dec() { _requests--;}

        };

        class C_listenner :public listenner, public ConnInfo {

            class listen_task :public task_obj {
            public:
                SOCKET _s;
                SOCKADDR_IN _addr[4];
                listen_task(C_listenner *listen_obj):_s(-1){
                    _conn = listen_obj;
                    listen_obj->inc();
                }
                ~listen_task() {(static_cast<C_listenner*>(_conn))->dec();}

                virtual void destroy() {

                    allocator_l0::allocator *alloc = (static_cast<C_listenner*>(_conn))->instance()->get_allocator();
                    this->~listen_task();
                    alloc->deallocate(this);
                }
                virtual const char *type() const { return "listen"; }
                static listen_task *create(C_listenner *conn) {
                    listen_task *ret = static_cast<listen_task *>( conn->instance()->get_allocator()->allocate(sizeof(listen_task)) );
                    if ( ret ) {
                        new ((void*)ret) listen_task(conn);
                    }
                    return ret;
                }
            };

            iocp_sock_poll *_sock_poll;
            SOCKET _socks[3];
            int _valid_conn;
            int _begin_index;
            virtual void *inst() {
                ConnInfo *instance = this;
                return instance;
            }
            virtual const char *type() const { return "listen"; }
            SOCKET get() {
                if ( _valid_conn == 0 ) return -1;
                _valid_conn--;
                SOCKET s = _socks[_begin_index];
                _begin_index++;
                if ( _begin_index == sizeof(_socks) / sizeof(_socks[0]) ) _begin_index = 0;
                return s;
            }
            virtual connection *accept() {
                if ( _valid_conn ) {
                    SOCKET s = get();
                    connection *conn = _sock_poll->connect(s);
                    if ( conn == 0 ) closesocket(s);
                    C_connection *conn_obj = static_cast<C_connection *>(conn);
                    conn_obj->read();
                    return conn;
                }
                return 0;
            }
        public:
            virtual void destroy() {
                _remoeved = true;
                if ( _requests && _s != INVALID_SOCKET ) 
                    CancelIo((HANDLE)_s);
                if ( _requests == 0 ) 
                    _sock_poll->destroy(this);
            }

            bool try_destroy() {
                if ( _remoeved && _requests == 0 ) {
                    destroy();
                    return true;
                }
                return false;
            }

            C_listenner(iocp_sock_poll *sock_poll, SOCKET s):_sock_poll(sock_poll), _begin_index(0), _valid_conn(0){
                _s = s;
                _sock_poll->bind(_s);
            }

            ~C_listenner() {
                if ( _s != INVALID_SOCKET ) closesocket(_s);
                _s = INVALID_SOCKET;
                while ( _valid_conn ) {
                    closesocket(get());
                }
            }
            bool exec() {
                if ( _remoeved ) return false;
                if ( _AcceptEx == 0 ) {
                    GUID guidAcceptEx = WSAID_ACCEPTEX;
                    DWORD dwBytes = 0;
                    if(WSAIoctl(_s,SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &guidAcceptEx,sizeof(guidAcceptEx),&_AcceptEx,sizeof(_AcceptEx), (LPDWORD)&dwBytes,NULL,NULL) != 0) {
                            return false;
                    }
                }

                while ( _requests + _valid_conn < 3 ) {
                    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if ( s == -1 ) return false;
                    dword_t cp_size = 0;
                    listen_task *task = listen_task::create(this);
                    if ( task == 0 ) {
                        closesocket(s); return false;
                    }
                    task->_s = s;
                    _AcceptEx(_s, s, (void*)&(task->_addr), 0, sizeof(SOCKADDR_IN) * 2, sizeof(SOCKADDR_IN) * 2, (LPDWORD)&cp_size, task);
                    if ( GetLastError() != ERROR_IO_PENDING ) {
                        printf("acceptex error %d\n", GetLastError());
                        task->destroy();
                        closesocket(s); return false;
                    }
                }
                return true;
            }
            iocp_sock_poll *instance() { return _sock_poll; }
            void parse_task(task_obj *task) {
                listen_task *listen_tsk = static_cast<listen_task *>(task);
                if ( _valid_conn == 0 ) _begin_index = 0;
                int index = (_valid_conn + _begin_index) % 3;
                _socks[index] = listen_tsk->_s;
                _valid_conn++;
            }
        };
        class C_connection :public connection, public ConnInfo {

        public:
            class connect_task :public task_obj {

            public:
                connect_task(C_connection *conn){
                    _conn = conn;
                    conn->inc();
                }
                ~connect_task(){(static_cast<C_connection*>(_conn))->dec();}
                virtual void destroy() {

                    allocator_l0::allocator *alloc = (static_cast<C_connection*>(_conn))->instance()->get_allocator();
                    this->~connect_task();
                    alloc->deallocate(this);
                }
                static connect_task *create(C_connection *conn) {
                    connect_task *ret = static_cast<connect_task *>( conn->instance()->get_allocator()->allocate(sizeof(connect_task)) );
                    if ( ret ) {
                        new ((void*)ret) connect_task(conn);
                    }
                    return ret;
                }

                virtual const char *type() const { return "connect"; }
            };
            bool create_connect() {

                assert(_task == 0);
                _task = connect_task::create(this);
                return _task != 0;
            }
            class write_task :public connect_task {
            public:
                container_l2::stream<1000> _send_cache;
//                char *_buffer;
                int _req_size;
                write_task(C_connection *conn):connect_task(conn), _req_size(0), _send_cache(conn->instance()->get_allocator()){}
                ~write_task(){}
                virtual void destroy() {

                    allocator_l0::allocator *alloc = (static_cast<C_connection*>(_conn))->instance()->get_allocator();
                    this->~write_task();
                    alloc->deallocate(this);
                }
                virtual const char *type() const { return "write"; }

                bool exec() {

                    if ( _send_cache.size() == 0 ) return false;
                    if ( _buffer.buf ) return false;
                    dword_t tsk_size = unistd_l0::Min<dword_t>(4000, _send_cache.size());
                    if ( tsk_size > 1024 ) tsk_size = 4000;
                    if ( 0 == (_buffer.buf = static_cast<char *>((static_cast<C_connection*>(_conn))->instance()->get_allocator()->allocate(tsk_size)) ) ) {
                        return false;
                    }
                    dword_t finished_size = 0;
                    _req_size = _buffer.len = _send_cache.peek(_buffer.buf, tsk_size);
                    if ( WSASend((static_cast<C_connection*>(_conn))->_s, &_buffer, 1, (LPDWORD)&finished_size, MSG_PARTIAL, this, 0) ) {

                        if ( GetLastError() == WSA_IO_PENDING ) {

                        }
                        else {
                            printf("send %d\n", GetLastError());
                            (static_cast<C_connection*>(_conn))->_state = 'b';
                        }
                    }

                    return (static_cast<C_connection*>(_conn))->_state == 'f';
                }
                void pase_result() {
                    _send_cache.pop_front(_cp_size);
                    if ( _buffer.buf ) (static_cast<C_connection*>(_conn))->instance()->get_allocator()->deallocate(_buffer.buf);
                    _buffer.buf = 0;
                }

                static write_task *create(C_connection *conn) {
                    write_task *ret = static_cast<write_task *>( conn->instance()->get_allocator()->allocate(sizeof(write_task)) );
                    if ( ret ) {
                        new ((void*)ret) write_task(conn);
                    }
                    return ret;
                }
            };
            class read_task :public connect_task {
            public:
                container_l2::stream<1000> _recv_cache;
//                char *_buffer;
                int _req_size;
                read_task(C_connection *conn)
                    :connect_task(conn), _req_size(0), _recv_cache(conn->instance()->get_allocator()){
                }

                ~read_task(){}
                virtual void destroy() {

                    allocator_l0::allocator *alloc = (static_cast<C_connection*>(_conn))->instance()->get_allocator();
                    this->~read_task();
                    alloc->deallocate(this);
                }
                virtual const char *type() const { return "read"; }

                bool exec() {

                    if ( _buffer.buf ) return false;
                    _req_size = 8000;
                    if ( 0 == (_buffer.buf = static_cast<char *>((static_cast<C_connection*>(_conn))->instance()->get_allocator()->allocate(_req_size)) ) ) {

                        return false;
                    }
                    dword_t finished_size = 0;
                    dword_t flag = MSG_PARTIAL;
                    _buffer.len = _req_size;
                    WSARecv((static_cast<C_connection*>(_conn))->_s, &_buffer, 1, (LPDWORD)&finished_size, (LPDWORD)&flag, this, 0);
                    return (static_cast<C_connection*>(_conn))->_state == 'f';
                }

                void pase_result() {
                    if ( _cp_size )_recv_cache.push_back(_buffer.buf, _cp_size);
//                    _buffer.buf[_cp_size] = '\0';
//                    printf("%s\n", _buffer.buf);
                    if ( _buffer.buf ) (static_cast<C_connection*>(_conn))->instance()->get_allocator()->deallocate(_buffer.buf);
                    _buffer.buf = 0;
                }

                static read_task *create(C_connection *conn) {
                    read_task *ret = static_cast<read_task *>( conn->instance()->get_allocator()->allocate(sizeof(read_task)) );
                    if ( ret ) {
                        new ((void*)ret) read_task(conn);
                    }
                    return ret;
                }

            };

        private:
            iocp_sock_poll *_sock_poll;
        public:
            DWORD _failed_time;
            char _state;
            int _errcode;
            task_obj *_task;
            task_obj *_rtask;
            virtual void destroy() {
                _remoeved = true;
                if ( _requests == 0 ) 
                    _sock_poll->destroy(this);
                else 
                    CancelIo((HANDLE)_s);
            }
            bool try_destroy() {
                if ( _remoeved && _requests == 0 ) {
                    destroy();
                    return true;
                }
                return false;
            }
            virtual void *inst() {
                ConnInfo *instance = this;
                return instance;
            }
            virtual void change_state() { if (_state == 'i') _state = 'f'; }

            virtual const char *state(const char *query_type) {

                const char *ret = 0;
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
                    {
                        ret = "empty";
                        if ( _rtask == 0 ) return ret;
                        read_task *task = static_cast<read_task*>(_rtask);
                        if ( task->_recv_cache.size() ) ret = "ready";
                        return ret;
                    }
                case 'w': //"write"
                    return (_state == 'f')? "ready": "full";
                case 'e': //"error"
                    return ((_state == 'f') || (_state == 'i') )? "fine": "breaked";
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
                if ( _state != 'f' ) return 0;
                write_task *task = 0;
                if ( _task == 0 ) {
                    task = write_task::create(this);
                    if ( task == 0 ) return 0;
                }
                else {
                    assert ( *_task->type() == 'w' );
                    task = static_cast<write_task *>(_task);
                }
                task->_send_cache.push_back(data, len);

                task->exec();
                _task = task;
                return len;
            }
            virtual dword_t read(char *buf, dword_t len) {

                if ( _rtask == 0 ) return 0;
                read_task *rtask = static_cast<read_task *>(_rtask);
                return rtask->_recv_cache.pop_front(len, buf);
            }
            virtual dword_t write(container_l2::stream_wrapper &send_buf) {

                dword_t ret = 0;
                while ( ret < send_buf.size() ) {

                    char buffer[1024 * 100];
                    dword_t pop_size = send_buf.pop_front(sizeof(buffer), buffer);
                    dword_t top_send = write(buffer, pop_size);
                    ret += top_send;
                    if ( top_send < pop_size ) break;
                }
                return ret;
            }
            virtual dword_t read(container_l2::stream_wrapper &recv_buf) {
                dword_t ret = 0;
                char buffer[1024 * 100];
                if ( _rtask == 0 ) return 0;
                read_task *rtask = static_cast<read_task *>(_rtask);
                while ( dword_t size = rtask->_recv_cache.pop_front(sizeof(buffer), buffer) ) {
                    ret += size;
                    recv_buf.push_back(buffer, size);
                }
                return ret;
            }
            virtual SOCKET unbind() {
                return _sock_poll->unbind(this);
            }
            void read() {
                if ( _rtask ) return;
                if ( _state != 'f' ) return;
                read_task *task = read_task::create(this);
                _rtask = task;
                task->exec();
            }
        public:
            C_connection(iocp_sock_poll *sock_poll, SOCKET s) 
                :_sock_poll(sock_poll), _state('f'), _errcode(0), _task(0), _rtask(0) { _s = s;}
            ~C_connection() {
                SOCKET s = _sock_poll->unbind(this);
                if ( s != INVALID_SOCKET ) closesocket(s);
                if ( _task ) _task->destroy();
            }
            iocp_sock_poll *instance() { return _sock_poll; }
            void parse_write() {
                write_task *wtask = static_cast<write_task *>(_task);
                wtask->pase_result();
                if ( wtask->_cp_size == 0 ) _state = 'b';
                if ( _remoeved || wtask->_send_cache.size() == 0 || wtask->_cp_size == 0 ) {
                    _task->destroy();  _task = 0;
                }
                else wtask->exec();
            }
            void parse_read() {
                read_task *rtask = static_cast<read_task *>(_rtask);
                rtask->pase_result();
                if ( rtask->_cp_size == 0 ) _state = 'b';
                if ( _remoeved || rtask->_cp_size == 0 ) {
                    _rtask->destroy();
                    _rtask = 0;
                }
                else rtask->exec();
            }
        };
        typedef container_l0::rbtree<sock_obj *> RBTree;

        HANDLE _iocp;
    protected:
        friend class C_listenner;
        friend class C_connection;
        SOCKET unbind(ConnInfo *conn) {
            SOCKET ret = conn->_s;
            conn->_s = INVALID_SOCKET;
            return ret;
        }
        bool bind(SOCKET s) {
            sockblock(s, false);
            HANDLE h_iocp = ::CreateIoCompletionPort((HANDLE)s, _iocp, (ULONG_PTR)0, 0);
            assert(h_iocp == _iocp);
            return true;
        }
        void destroy(C_listenner *Listenner) {
            Listenner->~C_listenner();
            _allocator->deallocate(Listenner);
        }
        void destroy(C_connection *conn) {
            conn->~C_connection();
            _allocator->deallocate(conn);
        }
        C_connection *create_connecttor(SOCKET s) {

            C_connection *conn = static_cast<C_connection *>(_allocator->allocate(sizeof(C_connection)));
            if ( conn ) {
                ::new ((void*)conn) C_connection(this, s);
                conn->_state = 'i';
            }
            return conn;
        }

    public:
        HANDLE get_iocp() { return _iocp; }
        iocp_sock_poll(allocator_l0::allocator *Allocator) :allocator_l0::contain_base(Allocator) {

            _iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE , NULL, 0, 0 );
        }
        ~iocp_sock_poll() {
            if (_iocp ) {
                sock_obj *obj_list[1];
                query(obj_list, 1);
                CloseHandle(_iocp);
            }
        }
        virtual void destroy() {
            sock_obj *obj_list[100];
            query(obj_list, 100);
            allocator_l0::allocator *alloc = _allocator;
            this->~iocp_sock_poll();
            alloc->deallocate(this);
        }
        virtual connection *connect(const char *peer_addr, const char *local_addr, dword_t time_out_msec) {

            _SERVICE_ADDR peer(peer_addr, 0);
            _SERVICE_ADDR local(local_addr, 0);
            if ( ! peer.valid() ) return 0;
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if ( s == INVALID_SOCKET ) return 0;

            if ( ConnectEx == 0 ) {

                DWORD dwBytes;
                GUID guidConnectEx=WSAID_CONNECTEX;
                if(SOCKET_ERROR==WSAIoctl(s,SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &guidConnectEx,sizeof(guidConnectEx),&ConnectEx,sizeof(ConnectEx),(LPDWORD)&dwBytes,NULL,NULL))
                {
                    closesocket(s); return 0;
                }
            }
            network_l3::set_reuse(s, true);

            SocketAddress localAddr( AF_INET, htons(local.port), htonl(local.ip) );
            ::bind(s, (sockaddr*)&localAddr, sizeof(localAddr));
            sockblock(s, false);

            C_connection *conn = create_connecttor(s);
            if ( conn == 0 ) {
                closesocket(s); return 0;
            }
            conn->_state = 'i';
            conn->_failed_time = GetTickCount() + time_out_msec;

            sock_obj *obj = conn;
            HANDLE h_iocp = ::CreateIoCompletionPort((HANDLE)s, _iocp, (ULONG_PTR)0, 0);
            assert(h_iocp == _iocp);

            SocketAddress remoteAddr(AF_INET, htons(peer.port), htonl(peer.ip));

            DWORD cp_size = 0;
            conn->create_connect();

            if ( ! ConnectEx(s, (sockaddr*)&remoteAddr, sizeof(remoteAddr), 0, 0, (LPDWORD)&cp_size, conn->_task) ) {
//                printf("ConnectEx ret %d\n", GetLastError());
            }
            return conn;
        }
        virtual connection *connect(SOCKET s) {

            C_connection *conn = static_cast<C_connection *>(_allocator->allocate(sizeof(C_connection)));
            if ( conn == 0 ) return 0;
            ::new ((void*)conn) C_connection(this, s);
            if ( bind(conn->_s) ) return conn;

            conn->_s = -1;
            conn->~C_connection();
            _allocator->deallocate(conn);

            return 0;
        }
        virtual listenner *listen(const char *local_addr) {

            _SERVICE_ADDR addr(local_addr, 0);
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
            if ( sock == INVALID_SOCKET ) return 0;

            network_l3::set_reuse(sock, true);
            sockblock(sock, false);

            SocketAddress listen_addr(AF_INET, htons(addr.port), htonl(addr.ip));
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
                listen_obj->exec();
            }
            else closesocket(sock);
            return listen_obj;
        }
        typedef container_l2::map<sock_obj *, char> ConnMap;
        sock_obj *pre_parse(task_obj *task) {

            switch ( *(task->type()) ) {
            case 'l':
                {
                    C_listenner *listen_obj = static_cast<C_listenner *>(task->_conn);
                    listen_obj->parse_task(task);
                    task->destroy();
                    if ( listen_obj->try_destroy() ) return 0;
                    if ( listen_obj->_remoeved ) return 0;
                    listen_obj->exec();
                    return listen_obj;
                }
                break;
            case 'c':
                {
                    C_connection *conn = static_cast<C_connection *> (task->_conn);
                    conn->_state = in_connect(conn->_s) ? 'f': 'b';
                    task->destroy();
                    conn->_task = 0;
                    if ( conn->_state == 'f' ) conn->read();
                    return conn;
                }
            case 'r':
                {
                    C_connection *conn = static_cast<C_connection *> (task->_conn);
                    assert(conn->_rtask == task);
                    conn->parse_read();
                    if ( conn->try_destroy() ) return 0;
                    return conn;
                }
            case 'w':
                {
                    C_connection *conn = static_cast<C_connection *> (task->_conn);
                    assert( conn->_task == task );
                    conn->parse_write();
                    if ( conn->try_destroy() ) return 0;
                    return conn;
                }
            default:
                break;
            }
            assert(false);
            return 0;
        }
        virtual dword_t query(sock_obj **obj_list, dword_t list_count) {


            if ( list_count < 1 ) return 0;

            task_obj *task;

            if ( ( task = query_task(_iocp) ) == 0 ) return 0;

            dword_t ret = 0;
            sock_obj *conn;
            if ( conn = pre_parse(task) ) obj_list[ret++] = conn;

            return ret;
        }
    };
    inline 
        tcp_sock_poll *tcp_sock_poll::create(allocator_l0::allocator *Allocator) {
            iocp_sock_poll *inst = static_cast<iocp_sock_poll *>(Allocator->allocate(sizeof(iocp_sock_poll)));
            new ((void*)inst) iocp_sock_poll(Allocator);
            return inst;
    }
}