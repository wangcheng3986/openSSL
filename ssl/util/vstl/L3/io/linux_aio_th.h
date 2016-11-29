#pragma once
#include "./aio_base.h"
#include "../../L0/containers/pool.h"
#include "../../L0/abstract/unistd.h"
#include "../../L3/system/thread.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libaio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

namespace storage_l3 {
    class linux_aio :public aio ,public thread_l3::service_base, public allocator_l0::contain_base{
//        allocator_l0::allocator *_allocator;
        io_context_t _aio_ctx;
        container_l0::pool<> _reqs;
        thread_l3::service_frame _thread;
        virtual bool process() {
            bool ret = false;
            while ( container_l0::pool_node *req = _reqs.get() ) {
                my_task *task = static_cast<my_task *>(req);
                exec(task);
                ret = true;
            }
            return ret;
        }
        struct io_event _evt_list[25];

        class my_aio_file;
        class my_task :public aio_task, public container_l0::pool_node, public iocb {
            my_aio_file *_file;
            struct iocb *io_data() const { 
                struct iocb *ret = const_cast<my_task*>(this);
                return ret;
            }
        public:
            int _trans_size;
            int _error;
            struct iocb *_p;
            ret_code _state;
            struct iocb **cb() { return &_p; }
            virtual void destroy() {

                allocator_l0::allocator *alloc = _file->get_allocator();
                this->~my_task();
                alloc->deallocate(this);
            }
            virtual void cancel() {
                if ( ( _state == io_canceled ) || ( _state == io_success ) ) return;
            }
            virtual ret_code state() const { return _state; }
            virtual int err_code() const { return _error; }
            virtual int trans_size() const { return _trans_size; }
            virtual char *data() const { return (char*)(io_data()->u.c.buf); }
            virtual aio_file *owner() { return _file; }

            my_task(char *data, dword_t size, qword_t offset, bool is_read, my_aio_file *file, int fd)
                :_file(file), _state(io_wait), _trans_size(0), _error(0), _p(this){

                    if ( is_read ) {
                        io_prep_pread(io_data(), fd, data, size, offset);
                    }
                    else {
                        io_prep_pwrite(io_data(), fd, data, size, offset);  
                    }
            }
            ~my_task() {

            }
        };
        class my_aio_file :public aio_file {
            linux_aio *_inst;
            int _fd;
        public:
            int fd() const { return _fd; }

            linux_aio *inst() { return _inst;}

            my_aio_file(const char *file_name, int open_mode, linux_aio *inst) 
                :_inst(inst) {
                    _fd = (open_mode & O_CREAT)
                        ? open(file_name, open_mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
                        : open(file_name, open_mode);
            }

            ~my_aio_file() { if ( _fd != -1 ) close(_fd); }

            allocator_l0::allocator *get_allocator() { return _inst->get_allocator(); }

            virtual void destroy() { 
                allocator_l0::allocator *alloc = get_allocator();
                this->~my_aio_file();
                alloc->deallocate(this);
            }

            virtual aio_task *read(char *data, dword_t size, qword_t offset) {

                if (_fd == -1 ) return 0;
                my_task *ret = static_cast<my_task *>( get_allocator()->allocate(sizeof(my_task)) );
                new ((void*)ret) my_task(data, size, offset, true, this, _fd);
                _inst->put(ret);
                return ret;
            }
            virtual aio_task *write(char *data, dword_t size, qword_t offset) {

                if (_fd == -1 ) return 0;
                my_task *ret = static_cast<my_task *>( get_allocator()->allocate(sizeof(my_task)) );
                new ((void*)ret) my_task(data, size, offset, false, this, _fd);
                _inst->put(ret);
                return ret;
            }
            virtual long long size() {
                if ( _fd == -1 ) return -1;
                struct stat ret;
                if ( fstat(_fd, &ret) == 0 ) return ret.st_size;
                return -1;
            }

        };
    protected:
        friend class my_aio_file;

//        allocator_l0::allocator *get_allocator() { return _allocator; }
    public:
        linux_aio(allocator_l0::allocator *Allocator) :allocator_l0::contain_base(Allocator),_aio_ctx(0), _thread(this) {
            int ret = io_setup(1020, &_aio_ctx);
            if ( ret != 0 ) {
                printf("io_setup error %s\n", strerror(errno));
            }
            _thread.start(1);
        }
        ~linux_aio() {
            _thread.stop();
            process();
            io_destroy(_aio_ctx);
        }
        bool exec(my_task *task) {
            int ret = io_submit(_aio_ctx, 1, task->cb());
            if ( ret != 1 ) {
                printf("io_submit ret %d %s\n", ret, strerror(errno));
            }
            return ret == 1;
        }
        void put(my_task *task) { _reqs.put(task); }
        virtual void destroy() {
            allocator_l0::allocator *alloc = _allocator;
            this->~linux_aio();
            alloc->deallocate(this);
        }
        virtual aio_file *create(const char *file_name, int open_mode) {
            my_aio_file *ret = static_cast<my_aio_file *>( _allocator->allocate(sizeof(my_aio_file)) );
            new ((void*)ret) my_aio_file(file_name, open_mode, this);
            return ret;
        }
        virtual dword_t query(aio_task **task_list, dword_t count) {

            count = unistd_l0::Min<dword_t>(count, sizeof(_evt_list) / sizeof(_evt_list[0]));
            int ret = io_getevents(_aio_ctx, 0, count, _evt_list, NULL);
            if ( ret <= 0 ) return 0;
            printf("io_getevents ret %d\n", ret);
            for ( dword_t i = 0; i < ret; i++ ) {
                struct iocb * cb = static_cast<struct iocb *>(_evt_list[i].obj);
                my_task *task = static_cast<my_task *>(cb);
                task->_trans_size = _evt_list[i].res;
                task->_state = static_cast<aio_task::ret_code>(_evt_list[i].res2);
                task_list[i] = task;
            }

            return ret;
        }

    };
    inline 
        aio *aio::create(allocator_l0::allocator *Allocator) {
            linux_aio *inst = static_cast<linux_aio *>(Allocator->allocate(sizeof(linux_aio)));
            new ((void*)inst) linux_aio(Allocator);
            return inst;
    }

}