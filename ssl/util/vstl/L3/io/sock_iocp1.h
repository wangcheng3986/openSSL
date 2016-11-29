#pragma once
#include "../../L0/abstract/type.h"
#include "../../L0/abstract/unistd.h"
#include "../../L0/containers/rbtree.h"
#include "../../L0/abstract/allocator.h"
#include "../../L2/buffer/stream.h"
#include "win_epoll.h"
#pragma warning (disable:4311 4312 4200 4996)
#include "Mswsock.h"
static LPFN_CONNECTEX ConnectEx = 0;
static LPFN_ACCEPTEX _AcceptEx = 0;
#include <new>
namespace socket_io_l3 {
#ifndef MY_ERROR_NUM
#define MY_ERROR_NUM
    static
        int error_num()
    {
#ifdef _WIN32
        return GetLastError();
#else
        return errno;
#endif
    }
#endif


    using namespace type_l0;

    class sock_obj {
    public:
        SOCKET _s;
        int _tasks;
        char _remoeved;
        sock_obj():_s(-1), _tasks(0){}
        ~sock_obj() { if ( _s > -1 ) closesocket(_s); }

        virtual void destroy() = 0;
        virtual const char *type() const = 0;
        virtual void *inst() = 0;
    };

    struct task_obj : public OVERLAPPED {

        WSABUF _buffer; //任务数据
        dword_t _cp_size;//完成字节数
        sock_obj *_conn;//连接对象
        char _task_type;//任务类型
        task_obj():_cp_size(0), _conn(0) {
            OVERLAPPED *p = this;
            memset(p, 0, sizeof(*p));
        }
        virtual void destroy() = 0;
        virtual const char *type() const = 0;
    };
    class connection :public sock_obj{
    public:
        virtual const char *state(const char *state_type) = 0;//"conn"  => return: "connecting", "fine", "breaked", "closed"; 
        //"read"  => "empty", "ready"; 
        //"write" => "ready", "full";
        virtual dword_t write(const char *data, dword_t len) = 0;
        virtual dword_t read(char *buf, dword_t len) = 0;
        virtual dword_t write(buffer_l2::stream_wrapper &send_buf) = 0;
        virtual dword_t read(buffer_l2::stream_wrapper &recv_buf) = 0;
        virtual dword_t ctrl(const char *req, char *ack) = 0; //error code
        virtual void change_state() = 0;
        virtual SOCKET unbind() = 0; //socket 解除绑定
    };
    class listenner :public sock_obj {
    public:
        virtual connection *accept() = 0;
    };
    class tcp_sock_poll {
        C_AutoNetwork _auto;

    public:
        virtual void destroy() = 0;
        virtual connection *connect(const char *peer_addr, const char *local_addr, dword_t time_out_msec) = 0;
        virtual connection *connect(SOCKET sock_fd) = 0;//绑定一个现有socket
        virtual listenner *listen(const char *local_addr) = 0;
        virtual dword_t query(sock_obj **obj_list, dword_t list_count) = 0;
        static tcp_sock_poll *create(abstract_l0::allocator *Allocator);
    };
    class iocp_sock_poll :public tcp_sock_poll {

        abstract_l0::allocator *_allocator;

        class C_listenner :public listenner {

            class listen_task :public task_obj {

            public:
                SOCKET _s;
                listen_task(C_listenner *listen_obj):_conn(listen_obj), _s(-1){
                    _conn->inc();
                }
                ~listen_task() {_conn->dec();}
                virtual void destroy() {

                    abstract_l0::allocator *alloc = _listenner->instance()->get_allocator();
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
                if ( _s != -1 ) closesocket(_s);
                _s = -1;
                _remoeved = true;
                if ( _req_count ) 
                    CancelIo((HANDLE)_s);
                else 
                    _sock_poll->destroy(this);
            }

            int _req_count;

            C_listenner(iocp_sock_poll *sock_poll, SOCKET s):_sock_poll(sock_poll), _req_count(0), _begin_index(0), _valid_conn(0){
                _s = s;
                _sock_poll->bind(this);
            }

            ~C_listenner() {
                while ( _valid_conn ) {
                    closesocket(get());
                }
            }
            bool exec() {
                if ( _AcceptEx == 0 ) {
                    GUID guidAcceptEx = WSAID_ACCEPTEX;
                    DWORD dwBytes = 0;
                    if(WSAIoctl(_s,SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &guidAcceptEx,sizeof(guidAcceptEx),&_AcceptEx,sizeof(_AcceptEx), (LPDWORD)&dwBytes,NULL,NULL) != 0) {
                            return false;
                    }
                }

                while ( _req_count + _valid_conn < 3 ) {
                    SOCKADDR_IN addr[4] = {0};
                    int addr_len = sizeof(addr) * 4;
                    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if ( s == -1 ) return false;
                    dword_t cp_size = 0;
                    listen_task *task = listen_task::create(this);
                    if ( task == 0 ) {
                        closesocket(s); return false;
                    }
                    task->_s = s;
                    _AcceptEx(_s, s, (void*)addr, addr_len, sizeof(addr) / 2, sizeof(addr) / 2, (LPDWORD)&cp_size, task);
                    if ( GetLastError() != ERROR_IO_PENDING ) {
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
            void inc() { _req_count++;}
            void dec() { _req_count--;}
        };
        class C_connection :public connection {

        public:
            class connect_task :public task_obj {

            protected:
                C_connection *_conn;
            public:
                connect_task(C_connection *conn):_conn(conn){conn->inc();}
                ~connect_task(){_conn->dec();}
                virtual void destroy() {

                    abstract_l0::allocator *alloc = _conn->instance()->get_allocator();
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
                buffer_l2::stream<1000> _send_cache;
                char *_buffer;
                int _req_size;
                write_task(C_connection *conn):connect_task(conn), _buffer(0), _req_size(0), _send_cache(conn->instance()->get_allocator()){}
                ~write_task(){}
                virtual void destroy() {

                    abstract_l0::allocator *alloc = _conn->instance()->get_allocator();
                    this->~write_task();
                    alloc->deallocate(this);
                }
                virtual const char *type() const { return "write"; }

                bool exec() {

                    if ( _send_cache.size() == 0 ) return false;
                    if ( _buffer ) return false;
                    dword_t tsk_size = unistd_l0::Min<dword_t>(4000, _send_cache.size());
                    if ( tsk_size > 1024 ) tsk_size = 4000;
                    _buffer = static_cast<char *>(_conn->instance()->get_allocator()->allocate(tsk_size));
                    if ( _buffer == 0 ) return false;
                    dword_t finished_size = 0;
                    _req_size = _send_cache.peek(_buffer, tsk_size);
                    WSABUF send_buf = {_req_size, _buffer};
                    if ( WSASend(_conn->_s, &send_buf, 1, (LPDWORD)&finished_size, MSG_PARTIAL, this, 0) ) {

                        if ( GetLastError() == WSA_IO_PENDING ) {

                        }
                        else {
                            printf("send %d\n", GetLastError());
                            _conn->_state = 'b';
                        }
                    }

                    return _conn->_state == 'f';
                }
                void pase_result(dword_t finished_size) {
                    _send_cache.pop_front(finished_size);
                    if ( _buffer ) _conn->instance()->get_allocator()->deallocate(_buffer);
                    _buffer = 0;
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
                buffer_l2::stream<1000> _recv_cache;
                char *_buffer;
                int _req_size;
                bool _in_req;
                read_task(C_connection *conn)
                    :connect_task(conn), _buffer(0), _req_size(0), _recv_cache(conn->instance()->get_allocator()), _in_req(false){}
                ~read_task(){}
                virtual void destroy() {

                    abstract_l0::allocator *alloc = _conn->instance()->get_allocator();
                    this->~read_task();
                    alloc->deallocate(this);
                }
                virtual const char *type() const { return "read"; }

                bool exec() {

                    if ( _buffer ) return false;
                    _req_size = 8000;
                    _buffer = static_cast<char *>(_conn->instance()->get_allocator()->allocate(_req_size));
                    if ( _buffer == 0 ) return false;
                    dword_t finished_size = 0;
                    dword_t flag = MSG_PARTIAL;
                    WSABUF recv_buf = {_req_size, _buffer};
                    //                    while (
                    WSARecv(_conn->_s, &recv_buf, 1, (LPDWORD)&finished_size, (LPDWORD)&flag, this, 0);
                    //                        == 0) {
                    //                        _recv_cache.push_back(_buffer, finished_size);
                    //                    }
                    //                    if ( GetLastError() == WSA_IO_PENDING ) {

                    //                    }
                    //                    else {
                    //                        printf("error %d\n", GetLastError());
                    //                        _conn->_state = 'b';
                    //                    }

                    return _conn->_state == 'f';
                }

                void pase_result(dword_t finished_size) {
                    if ( finished_size )_recv_cache.push_back(_buffer, finished_size);
                    _buffer[finished_size] = '\0';
                    printf("%s\n", _buffer);
                    if ( _buffer ) _conn->instance()->get_allocator()->deallocate(_buffer);
                    _buffer = 0;
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
            dword_t _tasks;
            DWORD _failed_time;
            char _state;
            char _read_state;
            char _write_state;
            int _errcode;
            task_obj *_task;
            task_obj *_rtask;
            virtual void destroy() {
                _remoeved = true;
                if ( _tasks == 0 ) 
                    _sock_poll->destroy(this);
                else 
                    CancelIo((HANDLE)_s);
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


            virtual const char *type() const { return "c"; }

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
            virtual dword_t write(buffer_l2::stream_wrapper &send_buf) {

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
            virtual dword_t read(buffer_l2::stream_wrapper &recv_buf) {
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
                :_sock_poll(sock_poll), _state('f'),_write_state('n'), _read_state('e'),_errcode(0), _task(0), _rtask(0) { _s = s;}
            ~C_connection() {
                SOCKET s = _sock_poll->unbind(this);
                if ( _task ) _task->destroy();
            }
            void inc() { _tasks++; }
            void dec() { _tasks--; }
            iocp_sock_poll *instance() { return _sock_poll; }
            void parse_write(dword_t size) {
                write_task *wtask = static_cast<write_task *>(_task);
                wtask->pase_result(size);
                if ( size == 0 ) _state = 'b';
                if ( _remoeved || wtask->_send_cache.size() == 0 || size == 0 ) {
                    _task->destroy();
                    _task = 0;
                }
                else wtask->exec();
            }
            void parse_read(dword_t size) {
                read_task *rtask = static_cast<read_task *>(_rtask);
                rtask->pase_result(size);
                if ( size == 0 ) _state = 'b';
                if ( _remoeved || size == 0 ) {
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
        SOCKET unbind(sock_obj *conn) {
            SOCKET ret = conn->_s;
            return ret;
        }
        bool bind(sock_obj *conn) {
            sockblock(conn->_s, false);
            HANDLE h_iocp = ::CreateIoCompletionPort((HANDLE)conn->_s, _iocp, (ULONG_PTR)conn, 0);
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
        abstract_l0::allocator *get_allocator() { return _allocator; }
        HANDLE get_iocp() { return _iocp; }
        iocp_sock_poll(abstract_l0::allocator *Allocator) :_allocator(Allocator) {

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
            abstract_l0::allocator *alloc = _allocator;
            this->~iocp_sock_poll();
            alloc->deallocate(this);
        }
        virtual connection *connect(const char *peer_addr, const char *local_addr, dword_t time_out_msec) {

            _SERVICE_ADDR peer;
            _SERVICE_ADDR local;
            peer.init(peer_addr, 0);
            local.init(local_addr, 0);
            if ( ! peer.valid() ) return 0;
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if ( s == INVALID_SOCKET ) return 0;

            if ( ConnectEx == 0 ) {

                DWORD dwBytes;
                GUID guidConnectEx=WSAID_CONNECTEX;
                if(SOCKET_ERROR==WSAIoctl(s,SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &guidConnectEx,sizeof(guidConnectEx),&ConnectEx,sizeof(ConnectEx),(LPDWORD)&dwBytes,NULL,NULL))
                {
                    closesocket(s);
                    return 0;
                }
            }


            setReuse(s, true);
            if ( local.valid() ) {
                SOCKADDR_IN localAddr = {0};
                localAddr.sin_family = AF_INET;
                localAddr.sin_port = htons(local.port);
                localAddr.sin_addr.s_addr = htonl(local.ip);
                ::bind(s, (sockaddr*)&localAddr, sizeof(localAddr));
            }
            sockblock(s, false);

            C_connection *conn = create_connecttor(s);
            if ( conn == 0 ) {
                closesocket(s); return 0;
            }
            conn->_state = 'i';
            conn->_failed_time = GetTickCount() + time_out_msec;

            sock_obj *obj = conn;
            HANDLE h_iocp = ::CreateIoCompletionPort((HANDLE)s, _iocp, (ULONG_PTR)obj, 0);
            assert(h_iocp == _iocp);

            SOCKADDR_IN remoteAddr = {0};
            remoteAddr.sin_addr.s_addr = htonl(peer.ip);
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = htons(peer.port);

            DWORD cp_size = 0;
            conn->create_connect();

            ConnectEx(s, (sockaddr*)&remoteAddr, sizeof(remoteAddr), 0, 0, (LPDWORD)&cp_size, conn->_task);
            return conn;
        }
        virtual connection *connect(SOCKET s) {

            C_connection *conn = static_cast<C_connection *>(_allocator->allocate(sizeof(C_connection)));
            if ( conn == 0 ) return 0;
            ::new ((void*)conn) C_connection(this, s);
            if ( bind(conn) ) return conn;

            conn->_s = -1;
            conn->~C_connection();
            _allocator->deallocate(conn);

            return 0;
        }
        virtual listenner *listen(const char *local_addr) {

            _SERVICE_ADDR addr;
            addr.init(local_addr, 0);
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
            if ( sock == INVALID_SOCKET ) return 0;

            setReuse(sock, true);
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
                listen_obj->exec();
            }
            else closesocket(sock);
            return listen_obj;
        }
        virtual dword_t query(sock_obj **obj_list, dword_t list_count) {
            epoll_event evt_list[1000];
            dword_t use_count = (dword_t)unistd_l0::Min<size_t>(list_count, sizeof(evt_list)/sizeof(evt_list[0]));

            DWORD cp_size = 0;
            void *context = 0;
            OVERLAPPED *request = 0;

            dword_t ret = 0;
            while ( ::GetQueuedCompletionStatus( _iocp, (LPDWORD)&cp_size, (PULONG_PTR)&context, (LPOVERLAPPED*)&request, 0) && (ret == 0) ) {
                assert(request != 0);
                sock_obj *object = static_cast<sock_obj *>(context);
                task_obj *task = static_cast<task_obj *>(request);
                if ( *object->type() == 'l' ) {
                    C_listenner *listen_obj = static_cast<C_listenner *>(object);
                    assert (*task->type() == 'l');
                    listen_obj->parse_task(task);
                    task->destroy();
                    if ( ! listen_obj->_remoeved ) {
                        listen_obj->exec();
                    }
                    if ( listen_obj->_req_count == 0 && listen_obj->_remoeved ) listen_obj->destroy();
                }
                else {
                    C_connection *connection_obj = static_cast<C_connection *>(object);
                    switch ( *task->type() ) {
                    case 'c':
                        {
                            //连接任务
                            BOOL error_state;
                            int opt_len = sizeof(error_state);
                            if ( getsockopt(connection_obj->_s, SOL_SOCKET, SO_ERROR, (char*)&error_state, &opt_len) ) error_state = 1;

                            connection_obj->_state = error_state? 'b': 'f';
                            task->destroy();
                            connection_obj->_task = 0;
                            connection_obj->read();
                        }
                        break;
                    case 'w':
                        {
                            assert(connection_obj->_task == task);
                            connection_obj->parse_write(cp_size);
                        }
                        break;
                    case 'r':
                        {
                            assert(connection_obj->_rtask == task);
                            connection_obj->parse_read(cp_size);
                        }
                        break;
                    default:
                        assert(false);
                        break;
                    }
                    if (connection_obj->_remoeved ) {
                        if ( connection_obj->_tasks == 0 ) connection_obj->destroy();
                        continue;
                    }
                }
                obj_list[ret++] = object;
            }
            return ret;

        }
    };
    inline 
        tcp_sock_poll *tcp_sock_poll::create(abstract_l0::allocator *Allocator) {
            iocp_sock_poll *inst = static_cast<iocp_sock_poll *>(Allocator->allocate(sizeof(iocp_sock_poll)));
            new ((void*)inst) iocp_sock_poll(Allocator);
            return inst;
    }
}