#pragma once
#pragma warning ( disable : 4355)
#include "./aio_base.h"
#include "../../L0/containers/pool.h"
#include "../../L3/system/thread.h"
#include <assert.h>
#include <fcntl.h>
#include <Windows.h>
#include <time.h>
namespace storage_l3 {
    class iocp_aio :public aio ,public thread_l3::service_base, public allocator_l0::contain_base{
//        allocator_l0::allocator *_allocator;
        class my_aio_file;
        HANDLE _iocp;
        container_l0::pool<> _reqs;
        thread_l3::service_frame _thread;
        virtual bool process() {
            bool ret = false;
            while ( container_l0::pool_node *req = _reqs.get() ) {
                my_task *task = static_cast<my_task *>(req);
                task->exec();
                ret = true;
            }
            return ret;
        }

        class my_task :public aio_task, public container_l0::pool_node, public OVERLAPPED {
            my_aio_file *_file;

            char *_data;
            int _size;
            bool _read;
        public:
            ret_code _state;
            int _trans_size;
            int _err_code;

            virtual void destroy() {

                allocator_l0::allocator *alloc = _file->get_allocator();
                this->~my_task();
                alloc->deallocate(this);
            }
            virtual void cancel() {

            }
            virtual ret_code state() const { return _state; }
            virtual int err_code() const { return _err_code; }
            virtual int trans_size() const { return _trans_size; }
            virtual char *data() const { return _data; }
            virtual aio_file *owner() { return _file; }

            my_task(char *data, dword_t size, qword_t offset, bool is_read, my_aio_file *file)
                :_file(file), _state(io_wait), _trans_size(0), _size(size), _err_code(0), _read(is_read){

                    OVERLAPPED *ovlap = this;
                    memset(ovlap, 0, sizeof(*ovlap));
                    *((UINT64*)&(ovlap->Pointer)) = offset;
                    _data = data;
            }
            bool exec() {

                DWORD byte_complete = 0;

                BOOL op_code = _read 
                    ? ReadFile(_file->fd(), _data, _size, &byte_complete, static_cast<OVERLAPPED *>(this))
                    : WriteFile(_file->fd(), _data, _size, &byte_complete, static_cast<OVERLAPPED *>(this));

                if ( ! op_code ) {
                    if ( GetLastError() != ERROR_IO_PENDING ) {
                        printf("execute err %d", GetLastError());
                        return false;
                    }
                }
                return true;
            }
            ~my_task() {

            }
        };
        class my_aio_file :public aio_file {
            iocp_aio *_inst;
            HANDLE _hfile;
        public:
            HANDLE fd() const { return _hfile; }

            my_aio_file(HANDLE hfile, iocp_aio *inst) 
                :_inst(inst), _hfile(hfile) {}

            ~my_aio_file() { CloseHandle(_hfile); }

            allocator_l0::allocator *get_allocator() { return _inst->get_allocator(); }

            virtual void destroy() { 
                allocator_l0::allocator *alloc = _inst->get_allocator();
                this->~my_aio_file();
                alloc->deallocate(this);
            }

            virtual aio_task *read(char *data, dword_t size, qword_t offset) {
                my_task *ret = static_cast<my_task *>( get_allocator()->allocate(sizeof(my_task)) );
                new ((void*)ret) my_task(data, size, offset, true, this);
                _inst->put(ret);
                return ret;
            }
            virtual aio_task *write(char *data, dword_t size, qword_t offset) {
                my_task *ret = static_cast<my_task *>( get_allocator()->allocate(sizeof(my_task)) );
                new ((void*)ret) my_task(data, size, offset, false, this);
                clock_t start = clock();
                _inst->put(ret);
//                printf("WriteFile use %d\n", clock() - start);
                return ret;
            }
            virtual long long size() {
                LARGE_INTEGER ret;
                if ( GetFileSizeEx(_hfile, &ret) ) return ret.QuadPart;
                return -1;
            }

        };
    protected:
        friend class my_aio_file;
    public:
        iocp_aio(allocator_l0::allocator *Allocator) :contain_base(Allocator), _thread(this){
            _iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE , NULL, 0, 0 );
            _thread.start(1);
        }
        ~iocp_aio() {
            _thread.stop();
            process();
            if (_iocp ) CloseHandle(_iocp);
        }
        void put(my_task *task) { _reqs.put(task); }
        virtual void destroy() {
            allocator_l0::allocator *alloc = _allocator;
            this->~iocp_aio();
            alloc->deallocate(this);
        }
        virtual aio_file *create(const char *file_name, int open_mode) {

            DWORD dwDesiredAccess = ( open_mode & O_WRONLY ) 
                ? (GENERIC_READ | GENERIC_WRITE)
                :  GENERIC_READ;
            DWORD dwCreationDisposition = ( open_mode & O_CREAT ) ? OPEN_ALWAYS : OPEN_EXISTING;
            if ( open_mode & O_TRUNC ) dwCreationDisposition |= TRUNCATE_EXISTING;
            HANDLE hfile = CreateFileA(file_name, dwDesiredAccess, FILE_SHARE_READ, 0, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);

            if ( hfile == INVALID_HANDLE_VALUE ) {
                printf("err code = %d", GetLastError());
                return 0;
            }

            HANDLE iocp = ::CreateIoCompletionPort(hfile, _iocp, 0, 0);
            assert(iocp == _iocp);

            my_aio_file *ret = static_cast<my_aio_file *>( _allocator->allocate(sizeof(my_aio_file)) );
            new ((void*)ret) my_aio_file(hfile, this);
            return ret;
        }
        virtual dword_t query(aio_task **task_list, dword_t count) {

            DWORD task_size = 0;
            void *context = 0;
            OVERLAPPED *request = 0;
            if ( ! ::GetQueuedCompletionStatus( _iocp, &task_size, (PULONG_PTR)&context, (LPOVERLAPPED*)&request, 0) ) {

                DWORD error_code = GetLastError();
                if ( error_code != WAIT_TIMEOUT ) printf("iocp aio error %d\n", error_code);
                return 0;
            }
            assert(request != 0);
            my_task *task = static_cast<my_task *>(request);
            task->_trans_size = task_size;
            task->_state = aio_task::io_success;
            task_list[0] = task;
            return 1;
        }

    };
    inline 
        aio *aio::create(allocator_l0::allocator *Allocator) {
            iocp_aio *inst = static_cast<iocp_aio *>(Allocator->allocate(sizeof(iocp_aio)));
            new ((void*)inst) iocp_aio(Allocator);
            return inst;
    }

}