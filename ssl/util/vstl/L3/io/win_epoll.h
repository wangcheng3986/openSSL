#pragma once
//#pragma pack(push, 1)
#include "../../L0/containers/rbtree.h"
//#include "../../L1/allocator/easy_allocator.h"
#pragma warning (push)
#pragma warning (disable:4311 4312 4200 4996)
#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32")
#define strcasecmp stricmp
#define strncasecmp strnicmp
enum EPOLL_EVENTS
{
    EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
    EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
    EPOLLOUT = 0x004,
#define EPOLLOUT EPOLLOUT
    EPOLLRDNORM = 0x040,
#define EPOLLRDNORM EPOLLRDNORM
    EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
    EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
    EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
    EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG
    EPOLLERR = 0x008,
#define EPOLLERR EPOLLERR
    EPOLLHUP = 0x010,
#define EPOLLHUP EPOLLHUP
    EPOLLONESHOT = (1 << 30),
#define EPOLLONESHOT EPOLLONESHOT
    EPOLLET = (1 << 31)
#define EPOLLET EPOLLET
};

#ifndef EWOULDBLOCK
#define EWOULDBLOCK             WSAEWOULDBLOCK
#endif

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3
// 
// #ifndef SOCKET
// 
// #endif
typedef union epoll_data
{
    void *ptr;
    int fd;
    unsigned int u32;
    unsigned long long u64;
} epoll_data_t;

struct epoll_event
{
    epoll_event():events(0){}
    unsigned int events;
    epoll_data_t data;
};

class WinEpoll {

    typedef container_l0::rbtree<SOCKET> RBTree;
    class SocketTree :public RBTree {
    public:
        class sock_node :public RBTree::node {
            virtual const RBTree::KeyElement & key() const { return _key; }
        public:
            SOCKET _key;
            epoll_event _value;
        public:
            sock_node(SOCKET s)
                :RBTree::node(_key) ,_key(s){}
            sock_node(SOCKET s, const epoll_event &event_info)
                :RBTree::node(_key) ,_key(s), _value(event_info){}
            virtual ~sock_node(){}
        };
    public:
        SocketTree(){}
        ~SocketTree() {
            while (size()) {
                sock_node *the_node = static_cast<sock_node*>(first());
                remove(the_node);
                delete the_node;
            }
        }

        epoll_event &operator [] (SOCKET s) {

            RBTree::node *node = 0;
            if ( (node = find(s)) == 0 ) {
                node = new sock_node(s);
                insert(node);
            }
            sock_node *ep_node = static_cast<sock_node*>(node);
            return ep_node->_value;
        }
    };


    struct FD_SET_CLASS {

        unsigned int fd_count;
        SOCKET  fd_array[0];

        void clear() { fd_count = 0; }

        void insert(SOCKET fd) {

            fd_array[fd_count] = fd;
            fd_count++;
        }

        void erase(SOCKET fd) {

            for ( unsigned int i = 0; i < fd_count; i++ ) {

                if ( fd_array[i] == fd ) {

                    fd_count--;
                    fd_array[i] = fd_array[fd_count];
                    break;
                }
            }
        }

        bool exist(SOCKET fd) {

            for ( unsigned int i = 0; i < fd_count; i++ ) {

                if ( fd_array[i] == fd ) return true;
            }
            return false;
        }
    };

    int _max_fd;

    SocketTree _fd_map;
    FD_SET_CLASS *_read_set;
    FD_SET_CLASS *_write_set;
    FD_SET_CLASS *_error_set;


public:

    WinEpoll(int maxfd):_max_fd(maxfd) {

        _read_set = (FD_SET_CLASS*) new char[sizeof(SOCKET) * _max_fd + sizeof(FD_SET_CLASS)];
        _write_set = (FD_SET_CLASS*) new char[sizeof(SOCKET) * _max_fd + sizeof(FD_SET_CLASS)];
        _error_set = (FD_SET_CLASS*) new char[sizeof(SOCKET) * _max_fd + sizeof(FD_SET_CLASS)];
    }
    ~WinEpoll() {

        if ( _read_set ) delete[] (char*)_read_set;
        if ( _write_set ) delete[] (char*)_write_set;
        if ( _error_set ) delete[] (char*)_error_set;
    }

    bool ctl(int fd, int op, const epoll_event *evt_info) {

        RBTree::node *node = 0;
        if ( op == EPOLL_CTL_ADD ) {

            if ( (int)_fd_map.size() >= _max_fd ) return false;
            if ( node = _fd_map.find(fd) ) return false;
            _fd_map.insert(new SocketTree::sock_node(fd, *evt_info));
            return true;
        }
        else if ( op == EPOLL_CTL_MOD ) {

            if ( node = _fd_map.find(fd) ) {

                SocketTree::sock_node *ep_node = static_cast<SocketTree::sock_node*>(node);
                ep_node->_value = *evt_info;
                return true;
            }
            return false;
        }
        else if ( op == EPOLL_CTL_DEL ) {

            if ( node = _fd_map.find(fd) ) {
                SocketTree::sock_node *ep_node = static_cast<SocketTree::sock_node*>(node);
                _fd_map.remove(ep_node);
                delete ep_node;
            }
            return true;
        }
        else return false;
    }

    int wait(epoll_event *evt_list, int count, int time_out) {

        clearBuffer();
        timeval timeout = { time_out / 1000, (time_out % 1000) * 1000 };
        int max_nfds = 0;
        fd_set *read_set = 0;
        fd_set *write_set = 0;
        fd_set *error_set = 0;

        if ( _fd_map.size() ) {
            fill_fd();
            max_nfds = max_fd_in_buffer() + 1;
            _read_set->fd_count? (read_set = (fd_set*)_read_set) :0;
            _write_set->fd_count? (write_set = (fd_set*)_write_set) :0;
            _error_set->fd_count? (error_set = (fd_set*)_error_set) :0;
        }
        else return 0;
        int ret = select( max_nfds, read_set, write_set, error_set, &timeout);
        if ( ret <= 0 ) {
            if ( ret < 0 ) printf("select error %d", GetLastError());
            return ret;
        }

        return get_event(evt_list, count);
    }

private:

    int get_event(epoll_event *__events, int __maxevents) {

        SocketTree tmpMap;
        for ( unsigned int i = 0; i < _read_set->fd_count; i++ ) {

            epoll_event &event = tmpMap[_read_set->fd_array[i]];
            event.events |= EPOLLIN;
            event.data = _fd_map[_read_set->fd_array[i]].data;
        }

        for ( unsigned int i = 0; i < _write_set->fd_count; i++ ) {

            epoll_event &event = tmpMap[_write_set->fd_array[i]];
            event.events |= EPOLLOUT;
            event.data = _fd_map[_write_set->fd_array[i]].data;
        }

        for ( unsigned int i = 0; i < _error_set->fd_count; i++ ) {

            epoll_event &event = tmpMap[_error_set->fd_array[i]];
            event.events |= (EPOLLET | EPOLLERR | EPOLLHUP);
            event.data = _fd_map[_error_set->fd_array[i]].data;
        }

        int ret = 0;
        for ( RBTree::node *node = tmpMap.first(); node; node = node->next(), ret++ ) {

            SocketTree::sock_node *ep_node = static_cast<SocketTree::sock_node*>(node);
            __events[ret] = ep_node->_value;

        }
        return ret;
    }

    int max_fd_in_buffer() {

        unsigned int ret = 0;
        ret = unistd_l0::Max(_error_set->fd_count, _read_set->fd_count);
        return unistd_l0::Max(ret, _write_set->fd_count);

    }

    void fill_fd() {

        for ( RBTree::node *node = _fd_map.first(); node; node = node->next() ) {

            SocketTree::sock_node *ep_node = static_cast<SocketTree::sock_node*>(node);
            epoll_event & event = ep_node->_value;
            if ( event.events & EPOLLIN ) _read_set->insert(ep_node->_key);
            if ( event.events & EPOLLOUT ) _write_set->insert(ep_node->_key);
            if ( event.events & (EPOLLERR | EPOLLET | EPOLLHUP) ) _error_set->insert(ep_node->_key);
        }
    }

    void clearBuffer() {

        _error_set->clear();
        _read_set->clear();
        _write_set->clear();
    }

private:

};
inline int epoll_create (int __size)
{
    return (int) new WinEpoll(__size);
}

inline int epoll_ctl (int __epfd, int __op, SOCKET __fd, epoll_event *__event_t)
{

    if ( ((WinEpoll*)__epfd)->ctl((int)__fd, __op, __event_t) )	return 0;

    return -1;

}

inline int epoll_wait (int __epfd, epoll_event *__events, int __maxevents, int __timeout)
{
    return ((WinEpoll*)__epfd)->wait(__events, __maxevents, __timeout);
}

inline void epoll_release(int __epfd) 
{
    delete (WinEpoll*)__epfd;
}
#else 
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#ifndef _EP_RELEASE_
#define _EP_RELEASE_
inline void epoll_release(int __epfd) 
{
    close(__epfd);
//    delete (WinEpoll*)__epfd;
}
#endif
#ifndef _TICK_AND_ERROR_DEF
#define _TICK_AND_ERROR_DEF
static
int GetLastError()
{
    return errno;
}
static long long GetTickCount64()
{
    timeval tv;
    //	timezone tz;
    gettimeofday(&tv, NULL);

    return ((long long)tv.tv_sec) * 1000000 + tv.tv_usec;

}
static unsigned int GetTickCount()
{
    return (unsigned int)(GetTickCount64() / 1000);
}
#endif
#ifndef SOCKET
#define SOCKET int
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)

#define closesocket ::close
#define OUT
#define IN
#define SOCKET_ERROR (-1)
// #define max(a,b)    (((a) > (b)) ? (a) : (b))
// #define min(a,b)    (((a) < (b)) ? (a) : (b))

typedef struct sockaddr_in SOCKADDR_IN;

#endif

#endif

#include "socket_base.h"


#pragma warning(pop)

//#pragma pack(pop)
