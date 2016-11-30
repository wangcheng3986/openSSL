#pragma once
#pragma warning (disable :4996)

#include "../L0/abstract/unistd.h"

#ifdef _WIN32
static int getuid(){ return 0;}
#endif

#include "log.h"
#include "../L2/containers/string.h"
#include "../L2/containers/map.h"
#include "../L2/buffer/stream.h"
#include "../L3/io/aio.h"

namespace log_writer {
    struct task {
        char *_data;
        log_writer *_file;
    };
    class mgr {
    public:
        virtual storage_l3::aio *get_aio() = 0;
        virtual allocator_l0::allocator *get_allocator() = 0;
        virtual void on_create(log_writer *log_file) = 0;
        virtual void on_destroy(log_writer *log_file) = 0;
        virtual void on_task(storage_l3::aio_task *io_tsk, task &task_data) = 0;
    };
    class C_log_writer :public log_writer {
        log_level _level;
        storage_l3::aio_file *_file;
        unsigned long _last_clock;
        mgr *_mgr;
        int _tasks;
        bool _removed;
        int _file_count;
        long long _file_size;
        long long _file_size_max;
        container_l2::string _file_name;
        container_l2::stream<1000> _cache;


        static char *time_to_string(time_t time, char *buffer) {
            struct tm time_struct;
            time_struct = *localtime(&time);
            sprintf(buffer, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", 
                time_struct.tm_year + 1900, 
                time_struct.tm_mon + 1, 
                time_struct.tm_mday, 
                time_struct.tm_hour, 
                time_struct.tm_min, 
                time_struct.tm_sec
                );
            return buffer;
        }
        int put_head(char *buffer, log_level level)
        {
            timeval top_time;
            unistd_l0::get_system_time(&top_time);
            const char *level_name[] = {
                "OFF",
                "CRITICAL",
                "ERROR",
                "WARNING",
                "INFO",
                "VERBOSE",
                "DEBUG"
            };
            if ( level < log_critical ) return 0;
            if ( level > log_debug ) return 0;
//            buffer[0] = '\n';
            time_to_string(top_time.tv_sec, buffer);
            int rsize = (int)strlen(buffer);
            rsize += sprintf(buffer + rsize, ".%.3d (pid:%d,uid:%d) %s ", top_time.tv_usec / 1000, getpid(), getuid(), level_name[level]);
            return rsize;
        }
        bool write(const char *data, int len) {

            _cache.push_back(data, len);
            write();
            return true;
        }
        void reinit() {
            if ( _file ) {
                if ( _file_size < _file_size_max ) return;
                if ( _tasks ) return;
                _file->destroy();
                _file = 0;
                _file_size = -1;
            }

            bool geted = false;
            typedef container_l2::map<int, container_l2::string> stack_map;
            stack_map name_stack;
            char buf[10240];
            int size = sprintf(buf,"%s", _file_name.c_str());
            int findex = 0;
            if ( _file_count < 1 ) _file_count = 1;

            for ( ; findex < _file_count; findex++ ) {
                if ( findex ) sprintf(buf + size, ".%d", findex);
                name_stack.insert(findex, buf);
                if ( access(buf, 0) ) break; 
            }
            //达到数量上限，栈顶的删除
            stack_map::iterator it = name_stack.find(findex);
            if ( (findex == _file_count) && findex) {
                remove(it->second.c_str());
            }
            //重命名文件
            bool rename_failed = false;
            for ( ; name_stack.size() > 1 ; --findex) {
                stack_map::iterator it_top = it;
                --it;
                if ( rename(it->second.c_str(), it_top->second.c_str()) == -1 ) {
                    rename_failed = true; break;
                }
                name_stack.erase(it_top);
            }
            size = sprintf(buf, "%s", _file_name.c_str());
            if ( rename_failed ) {
                time_t curr_time = time(0);
                struct tm time_struct = *localtime(&curr_time);
                sprintf(buf + size, ".%.4d-%.2d-%.2d_%.2d%.2d%.2d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
            }
            _file = _mgr->get_aio()->create(buf, O_WRONLY | O_CREAT);
            if ( _file ) _file_size = _file->size();
        }
        void inc_task() { ++_tasks; }
    public:
        void write() {
            unsigned long top_time = clock();
            if ( top_time - _last_clock < 1000 && _cache.size() < 100 * 1024) return;

            reinit();

            if ( _file_size >= _file_size_max ) return;
            if ( _cache.size() == 0 ) return;
            _last_clock = top_time;
            type_l0::dword_t len = unistd_l0::Min<type_l0::dword_t>(_cache.size(), 1024 * 101);

            char *send_cache = static_cast<char *>( _mgr->get_allocator()->allocate(len) );
            if ( send_cache == 0 ) return;
            _cache.peek(send_cache, len, 0);
            task tsk = {send_cache, this};
            storage_l3::aio_task *io_task = _file->write(send_cache, len, _file_size);
            if ( io_task ) {
                _file_size += len;
                _mgr->on_task(io_task, tsk);
                inc_task();
                _cache.pop_front(len);
                return ;
            }
            _mgr->get_allocator()->deallocate(send_cache);
        }

        void dec_task() { --_tasks; }

        C_log_writer(const char *file_name, mgr *log_mgr)
            : _file_name(file_name, log_mgr->get_allocator())
            , _mgr(log_mgr)
            , _level(log_debug)
            , _tasks(0)
            , _file(0)
            , _removed(false)
            , _file_size(-1)
            , _file_size_max(1024 * 1024 * 10)
            , _file_count(10)
            , _cache(log_mgr->get_allocator())
            , _last_clock(0)

        {
            _file = _mgr->get_aio()->create(file_name, O_WRONLY | O_CREAT);
            if ( _file ) _file_size = _file->size();
            _mgr->on_create(this);
        }

        ~C_log_writer() {
            _mgr->on_destroy(this);
            if ( _file ) _file->destroy();
        }
        bool fine() const { return _file != 0; }

        bool try_destroy() {
            if ( _removed && _tasks == 0 ) {
                destroy(); return true;
            }
            return false;
        }
        virtual bool finished() { return _tasks == 0 && _cache.size() == 0; }

        virtual void destroy() {
            _removed = true;
            if ( _tasks == 0 ) {
                allocator_l0::allocator *alloc = _mgr->get_allocator();
                this->~C_log_writer();
                alloc->deallocate(this);
            }
        }
        virtual bool write(log_level level, const char *data, int len) {
            if ( _level <= log_off ) return false;
            if ( level > _level ) return false;
            if ( _file == 0 ) return false;
            char buffer[1024];
            int size = put_head(buffer, level);
            write(buffer, size);
            write(data, len);
            return true;
        }
        //设置,日志大小，文件个数,level
        virtual const char *ctrl(const char *cmd, char *data, int len) {

            if ( strcmp(cmd, "set_log_file_size") == 0 ) {

                _file_size_max = unistd_l0::atoi64(data);
            }
            else if ( strcmp(cmd, "set_log_file_count") == 0) {

                _file_count = atol(data);
            }
            else if ( strcmp(cmd, "set_log_level") == 0) {

                _level = *((log_level *)data);
            }
            return 0;
        }
        virtual int printf(log_level level, const char* format_desc, ...) {

            if ( _level <= log_off ) return 0;
            if ( level > _level ) return 0;
            if ( _file == 0 ) return 0;
            char buffer[1024 * 100];
            va_list args;
            va_start(args, format_desc);
            int ret = vsnprintf(buffer, sizeof(buffer) - 1, format_desc, args);
            va_end(args);
            write(level, buffer, ret);
            return ret;
        }
        virtual bool append(log_level level, const char *data, int len) {
            if ( _level <= log_off ) return false;
            if ( level > _level ) return false;
            if ( _file == 0 ) return false;
            return write(data, len);
        }
    };
    class C_log_creater :public log_creater, public mgr {
        allocator_l0::allocator *_allocator;
        storage_l3::aio *_aio;
        typedef container_l2::map<storage_l3::aio_task *, task> task_map;
        typedef container_l2::map<log_writer *, char> file_map;
        file_map _files;
        task_map _tasks;
        virtual storage_l3::aio *get_aio() { return _aio; }
        virtual allocator_l0::allocator *get_allocator() { return _allocator; }
        virtual void on_create(log_writer *log_file) { _files.insert(log_file, 'a'); }
        virtual void on_destroy(log_writer *log_file) {_files.erase(log_file); }
        virtual void on_task(storage_l3::aio_task *io_tsk, task &task_data) { _tasks.insert(io_tsk, task_data); }
    public:
        C_log_creater(allocator_l0::allocator *Allocate) 
            : _allocator(Allocate)
            , _aio(storage_l3::aio::create(Allocate))
            , _tasks(Allocate)
            , _files(Allocate)
        {

        }
        ~C_log_creater(){ _aio->destroy(); }

        virtual bool try_destroy() {
            while ( loop_check() );
            if ( _files.size() ) return false;
            allocator_l0::allocator *alloc = _allocator;
            this->~C_log_creater();
            alloc->deallocate(this);
            return true;
        }
        virtual log_writer *create(const char *filename) {

            C_log_writer *log_inst = static_cast<C_log_writer *>( _allocator->allocate(sizeof(C_log_writer)) );
            if ( log_inst == 0 ) return 0;
            new ((void *)log_inst) C_log_writer(filename, this);
            if ( log_inst->fine() ) return log_inst;
            log_inst->~C_log_writer();
            _allocator->deallocate(log_inst);
            return 0;
        }
        void parse_finished(storage_l3::aio_task *tsk) {

            task_map::iterator it = _tasks.find(tsk);
            assert(it != _tasks.end());
            it->first->destroy();
            C_log_writer *writer = static_cast<C_log_writer *>( it->second._file );
            _allocator->deallocate(it->second._data);
            _tasks.erase(it);
            writer->dec_task();
            writer->write();
            writer->try_destroy();
        }
        virtual bool loop_check() {

            bool ret = false;
            storage_l3::aio_task *task_list[100];
            while ( type_l0::dword_t size = _aio->query(task_list, 100) ) {
                ret = true;
                for ( type_l0::dword_t i = 0 ; i < size; i++ ) {

                    parse_finished(task_list[i]);
                }
                if ( size < 100 ) break;
            }
            for ( file_map::iterator it = _files.begin(); it != _files.end(); ++it ) {
                C_log_writer *writer = static_cast<C_log_writer *>(it->first);
                if ( ! writer->finished() ) writer->write();
            }
            return ret;
        }
    };
    inline
    log_creater *log_creater::create(allocator_l0::allocator *Allocate) {

        C_log_creater *ret = static_cast<C_log_creater *>( Allocate->allocate(sizeof(C_log_creater)) );
        if ( ret == 0 ) return 0;
        new ((void*)ret) C_log_creater(Allocate);
        return ret;
    }
}