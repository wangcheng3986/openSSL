#include "C_sockethandler.h"

type_l0::dword_t xadd(volatile type_l0::dword_t*word, type_l0::dword_t add)
{
    return interlockedExchangeAdd((volatile long *)word, (long)add);
}
C_sockethandler::C_sockethandler(void)
: _index(0), _exec_index(0)
{
}

C_sockethandler::~C_sockethandler(void)
{
    run();
}

class SSL_new_data :public param_object {
    void *_ctx;
    void *_ret;
    UINT64 _begin_time;
    UINT64 _end_time;
public:
    virtual void destroy() {
        allocator_l0::default_allocator tmp_alloc;
        this->~SSL_new_data();
        tmp_alloc.deallocate(this);
    }
    virtual void exec() {
        _handler->hand_SSL_new(_ctx, _ret, _begin_time, _end_time);
    }
    SSL_new_data(C_sockethandler *handler, void *ctx, void *ret, UINT64 start_time, UINT64 end_time)
        : _ctx(ctx), _ret(ret), _begin_time(start_time), _end_time(end_time)
    {
        _handler = handler;
        _id = _handler->get_id();
    }
    ~SSL_new_data() {}

};

class SSL_free_data : public param_object {
    void *_ssl;
    UINT64 _begin_time;
public:
    virtual void destroy() {
        allocator_l0::default_allocator tmp_alloc;
        this->~SSL_free_data();
        tmp_alloc.deallocate(this);
    }
    virtual void exec() {
        _handler->hand_SSL_free(_ssl, _begin_time);
    }

    SSL_free_data(C_sockethandler *handler, void *ssl, UINT64 start_time):_ssl(ssl), _begin_time(start_time)
    {
        _handler = handler;
        _id = _handler->get_id();
    }
    ~SSL_free_data(){}
};
class SSL_connect_data :public param_object {
    void *_ssl;
    int _ret;
    int _fd;
    UINT64 _begin_time;
    UINT64 _end_time;
public:
    virtual void destroy() {
        allocator_l0::default_allocator tmp_alloc;
        this->~SSL_connect_data();
        tmp_alloc.deallocate(this);
    }
    virtual void exec() {
        _handler->hand_SSL_connect(_ssl, _ret, _begin_time, _end_time, _fd);
    }
    SSL_connect_data(C_sockethandler *handler, void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd)
        :_ssl(ssl), _ret(ret), _begin_time(start_time), _end_time(end_time), _fd(fd)
    {
        _handler = handler;
        _id = _handler->get_id();
    }
    ~SSL_connect_data(){}
};


class SSL_read_data : public param_object {
    void * _ssl;
    int _ret;
    int _len;
    UINT64 _begin_time;
    UINT64 _end_time;
    void *_buf;
    void *get_addr() {
        return (void*)((char*)this + (sizeof(*this)));
    }
public:
    virtual void destroy() {
        allocator_l0::default_allocator tmp_alloc;
        this->~SSL_read_data();
        tmp_alloc.deallocate(this);
    }
    virtual void exec() {
        _handler->hand_SSL_read(_ssl, _buf, _len, _ret, _begin_time, _end_time);
    }
    SSL_read_data(C_sockethandler *handler, void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time)
        : _ssl(ssl), _len(num), _ret(ret), _begin_time(start_time), _end_time(end_time)
    {
        _handler = handler;
        _id = _handler->get_id();
        _buf = get_addr();
        memcpy(_buf, buf, num);
    }
    ~SSL_read_data() {}
};

class SSL_write_data : public param_object {
    void * _ssl;
    int _ret;
    int _len;
    UINT64 _begin_time;
    UINT64 _end_time;
    void *_buf;
    void *get_addr() {
        return (void*)((char*)this + (sizeof(*this)));
    }
public:
    virtual void destroy() {
        allocator_l0::default_allocator tmp_alloc;
        this->~SSL_write_data();
        tmp_alloc.deallocate(this);
    }
    virtual void exec() {
        _handler->hand_SSL_write(_ssl, _buf, _len, _ret, _begin_time, _end_time);
    }
    SSL_write_data(C_sockethandler *handler, void *ssl,const void *buf, int num, int ret, UINT64 start_time, UINT64 end_time)
        : _ssl(ssl), _len(num), _ret(ret), _begin_time(start_time), _end_time(end_time)
    {
        _handler = handler;
        _id = _handler->get_id();
        _buf = get_addr();
        memcpy(_buf, buf, num);
    }
    ~SSL_write_data() {}
};

class SSL_set_fd_data :public param_object {
    void *_ssl;
    int _fd;
    int _ret;
    UINT64 _begin_time;
public:
    virtual void destroy() {
        allocator_l0::default_allocator tmp_alloc;
        this->~SSL_set_fd_data();
        tmp_alloc.deallocate(this);
    }
    virtual void exec() {
        _handler->hand_SSL_set_fd(_ssl, _fd,  _ret, _begin_time);
    }
    SSL_set_fd_data(C_sockethandler *handler, void *s, int fd, int ret, UINT64 start_time)
        :_ssl(s), _fd(fd), _ret(ret), _begin_time(start_time)
    {
        _handler = handler;
        _id = _handler->get_id();
    }
    ~SSL_set_fd_data(){}
};


void C_sockethandler::on_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time)
{
    allocator_l0::default_allocator tmp_alloc;
    SSL_new_data *ret_val = static_cast<SSL_new_data *>( tmp_alloc.allocate( sizeof(SSL_new_data)) );
    new ((void*)ret_val) SSL_new_data(this, ctx, ret, start_time, end_time);
    _pool.put(ret_val);
}
void C_sockethandler::on_ssl_close(void *ssl, UINT64 start_time)
{
    allocator_l0::default_allocator tmp_alloc;
    SSL_free_data *ret_val = static_cast<SSL_free_data *>( tmp_alloc.allocate( sizeof(SSL_free_data)) );
    new ((void*)ret_val) SSL_free_data(this, ssl, start_time);
    _pool.put(ret_val);
}
void C_sockethandler::on_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd)
{
    allocator_l0::default_allocator tmp_alloc;
    SSL_connect_data * ret_val = static_cast<SSL_connect_data *>( tmp_alloc.allocate(sizeof(SSL_connect_data)) );
    new ((void*)ret_val) SSL_connect_data(this, ssl, ret, start_time, end_time, fd);
    _pool.put(ret_val);
}

void C_sockethandler::on_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time)
{
    allocator_l0::default_allocator tmp_alloc;
    void *data = tmp_alloc.allocate(sizeof(SSL_read_data) + num + 4);
    SSL_read_data *ret_val = static_cast<SSL_read_data *>( data );
    new ((void*)ret_val) SSL_read_data(this, ssl, buf, num, ret, start_time, end_time);
    _pool.put(ret_val);
}
void C_sockethandler::on_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time)
{
    allocator_l0::default_allocator tmp_alloc;
    void *data = tmp_alloc.allocate(sizeof(SSL_write_data) + num + 4);
    SSL_write_data *ret_val = static_cast<SSL_write_data *>( data );
    new ((void*)ret_val) SSL_write_data(this, ssl, buf, num, ret, start_time, end_time);
    _pool.put(ret_val);
}
void C_sockethandler::on_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time)
{
//    LOGD("on_ssl_set_fd--------------%d,%d,%d,%lld-------",s,fd,ret,start_time);
    allocator_l0::default_allocator tmp_alloc;
    SSL_set_fd_data * ret_val = static_cast<SSL_set_fd_data *>( tmp_alloc.allocate(sizeof(SSL_set_fd_data)) );
    new ((void*)ret_val) SSL_set_fd_data(this, s, fd, ret, start_time);
    _pool.put(ret_val);
}


void C_sockethandler::hand_SSL_new(void *ctx, void *ret, UINT64 start_time, UINT64 end_time)
{
//    LOGD("hand_SSL_new(%p, %p, %lld, %lld)", ctx, ret, start_time, end_time);

    _SSLMap::iterator it = _sslconns.find(ret);
    if ( it != _sslconns.end() ) {
        hand_SSL_free(ret, start_time);
    }
//    assert(it == _sslconns.end() );
    ConnectionInfo &conn = _sslconns[ret];
    conn._ssl = ret;
    conn._use_ssl = true;
    conn.start_time = start_time;
    conn._instance = this;

}
void C_sockethandler::hand_SSL_free(void *ssl, UINT64 start_time)
{
//    LOGD("hand_SSL_free(%p, %lld)", ssl, start_time);

        _SSLMap::iterator it = _sslconns.find(ssl);
    if ( it == _sslconns.end() ) {
        printf("ssl(%p) already released\n", ssl);
        return;
    }
    ConnectionInfo *conn_info = &(it->second);
    assert(conn_info->start_time <= start_time);
    conn_info->curr_time = start_time;
    on_user_close(conn_info->_fd, conn_info, -1);

}
void C_sockethandler::hand_SSL_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd)
{
//    LOGD("hand_SSL_connect(%p, %d, %lld, %lld, %d)", ssl, ret, start_time, end_time, fd);
    _SSLMap::iterator it = _sslconns.find(ssl);
    if ( it == _sslconns.end() ) {
        hand_SSL_new(0, ssl, start_time, start_time);
        it = _sslconns.find(ssl);
    }
    assert(it != _sslconns.end() ) ;
    ConnectionInfo *conn_info = &(it->second);
    if ( conn_info->_fd == -1 && fd != -1 ) conn_info->_fd = fd;
    conn_info->curr_time = end_time;
    if ( conn_info->connect_start == 0 ) 
        conn_info->connect_start = start_time;
    if ( ret > 0 ) {
        conn_info->connect_end = end_time;
        on_connect_finished(conn_info->_fd, conn_info, 0);
    }
}
void C_sockethandler::hand_SSL_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time)
{
//    LOGD("hand_SSL_read(%p, %p, %d, %d, %lld, %lld)", ssl, buf, num, ret, start_time, end_time);
    if ( ret <= 0 ){
		return;
	}
    _SSLMap::iterator it = _sslconns.find(ssl);
    assert(it != _sslconns.end() ) ;
	if(it == _sslconns.end()){
//        LOGD("hand_SSL_read ssl not found(%p, %p, %d, %d, %lld, %lld)\n", ssl, buf, num, ret, start_time, end_time);
		return;
	}
    ConnectionInfo *conn_info = &(it->second);
    conn_info->curr_time = end_time;
    on_read_end(conn_info->_fd, conn_info, (char*)buf, start_time, ret);

}
void C_sockethandler::hand_SSL_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time)
{
//    LOGD("hand_SSL_write(%p, %p, %d, %d, %lld, %lld)\n", ssl, buf, num, ret, start_time, end_time);
    if ( ret <= 0 ) {
		return;
	}
    _SSLMap::iterator it = _sslconns.find(ssl);
    assert(it != _sslconns.end() ) ;
	if(it == _sslconns.end()){
//        LOGD("hand_SSL_write ssl not found(%p, %p, %d, %d, %lld, %lld)\n", ssl, buf, num, ret, start_time, end_time);
		return;
	}
    ConnectionInfo *conn_info = &(it->second);
    conn_info->curr_time = end_time;
    on_write_end(conn_info->_fd, conn_info, (char*)buf, num, start_time, ret);

}
void C_sockethandler::hand_SSL_set_fd(void *s, int fd, int ret, UINT64 start_time)
{
//    LOGD("hand_SSL_set_fd(%p, %d, %d, %lld)", s, fd, ret, start_time);
    _SSLMap::iterator it = _sslconns.find(s);
    assert(it != _sslconns.end() ) ;
	if(it == _sslconns.end()){
//        LOGD("hand_SSL_set_fd ssl not found(%p, %d, %d, %lld)\n", s, fd, ret, start_time);
		return;
	}
    ConnectionInfo *conn_info = &(it->second);
    if ( conn_info->_fd > -1 && ret > 1 ) return;
    conn_info->curr_time = start_time;
    conn_info->_fd = fd;
}


type_l0::dword_t C_sockethandler::get_id()
{
    return xadd(&_index, 1);
}

void C_sockethandler::run()
{
    param_object *task = 0;
    while ( task = static_cast<param_object *>(_pool.get()) ) {
//        LOGD("----run---tasks id-%d",task->id());
        _tasks[task->id()] = task; 
    }

    while ( _tasks.size() ) {
//        LOGD("----run---_tasks-%d",_tasks.size());
        TaskMap::iterator it = _tasks.begin();
        if ( it->first != _exec_index ) break;
        task = it->second;
        _tasks.erase(it);
		if(!task){
			_exec_index++;
			continue;
		}
//        LOGD("thread----tid:%d/%d\n",gettid(), task->tid());
        task->exec();
        _exec_index++;
        task->destroy();
    }
}


void C_sockethandler::on_break(int fd, ConnectionInfo *conn)
{
    on_user_close(fd, conn, -3);
}

void C_sockethandler::on_user_close(int fd, ConnectionInfo *conn, int result_code)
{
    on_connect_finished(fd, conn, -1);
    conn->on_user_close(fd, result_code);
    if ( conn->_ssl ) _sslconns.erase(conn->_ssl);
}


void C_sockethandler::on_read_end(int fd, ConnectionInfo *conn_info, char *buf, UINT64 start_time, ssize_t ret)
{
    if ( conn_info->connect_end == 0 ) {
        conn_info->connect_end = start_time;
        on_connect_finished(fd, conn_info);
    }
    int read_size = -1;
    if ( ret > 0 ) 
    {
        read_size = ret;
        conn_info->on_down(fd, buf, read_size);
    }
    else
    {
        //recv 0, �Է��ر�����
        if ( ret == 0 )
        {
            on_user_close(fd, conn_info, -2);
        }
        //recv���󣬷��첽����
        else if ( conn_info->_err_num != EAGAIN && conn_info->_err_num != EWOULDBLOCK ) 
        {
            on_break(fd, conn_info);
        }
    }

}
void C_sockethandler::on_write_end(int fd, ConnectionInfo *conn_info, char *buf, size_t len, UINT64 start_time, ssize_t ret)
{
    //connect���ʱ�䶨λ
    if ( conn_info->connect_end == 0 ) {
        conn_info->connect_end = start_time;
        on_connect_finished(fd, conn_info);
    }
    int up_size = -1;
    if ( ret > 0 ) 
    {
        up_size = ret;
    }
    else 
    {
        if ( conn_info->_err_num == EAGAIN || conn_info->_err_num == EWOULDBLOCK) 
        {
            up_size = len;
        }
        else //send ʧ��
        {
            on_break(fd, conn_info);
        }
    }
    if ( up_size > 0 ) 
        conn_info->on_up(buf, up_size);
}

void C_sockethandler::on_connect_finished(int fd, ConnectionInfo *conn_info, int err_code)
{
    //LOGD("----on_connect_finished----");
    if ( conn_info->_connected ) return;
    conn_info->_connected = true;
    if ( conn_info->connect_start == 0 ) return;
    container_l2::smart_buffer<char, 1024 * 2> buffer;
    if ( conn_info->_ssl )
    {
        sprintf(  buffer.buffer()
                , "SSL|%d:%s|%lld,%lld|%d"
                , err_code, err_code?"fail":"success"
                , conn_info->connect_start / 1000
                , (conn_info->connect_end?conn_info->connect_end:conn_info->curr_time) / 1000
                , conn_info->_fd);
    }
    this->on_task(buffer.buffer());
}


