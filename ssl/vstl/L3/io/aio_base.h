#pragma once
#include "../../L0/abstract/allocator.h"
#include "../../L0/abstract/type.h"
namespace storage_l3 {
    using namespace type_l0;
    class aio_file;
    class aio_task {
    public:
        enum ret_code {
            io_success = 0,
            io_canceled = 1,
            write_faild = 2,
            read_faild = 3,
            io_wait = 4,
        };
        virtual void destroy() = 0;
        virtual void cancel() = 0;
        virtual ret_code state() const = 0;
        virtual int err_code() const = 0;
        virtual int trans_size() const = 0;
        virtual char *data() const = 0;
        virtual aio_file *owner() = 0;
    };
    class aio_file {
    public:
        virtual void destroy() = 0;
        virtual aio_task *read(char *data, dword_t size, qword_t offset) = 0;
        virtual aio_task *write(char *data, dword_t size, qword_t offset) = 0;
        virtual long long size() = 0;
    };
    class aio {
    public:
        virtual void destroy() = 0;
        virtual aio_file *create(const char *file_name, int open_mode) = 0;
        virtual dword_t query(aio_task **task_list, dword_t count) = 0;
        static aio *create(allocator_l0::allocator *Allocator);
    };
}