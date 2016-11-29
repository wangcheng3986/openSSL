
#pragma once
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#define O_BINARY 0
#else
#include <io.h>
#endif
#pragma warning(disable : 4996)
namespace storage_l3 {

    static unsigned long long file_size(const char *_file_name, bool &_success) 
    {
        int fd = open(_file_name, O_RDONLY | O_BINARY);
        if ( fd != -1 ) {
            struct stat _file_stat = {0};
            fstat(fd, &_file_stat); close(fd);
            _success = true;
            return _file_stat.st_size;
        }
        else _success = false;
        return 0;
    }

    class file_access {
        FILE *_fp;
        long long _f_size;
    public:
        file_access(const char *fname, const char *acc)
            :_fp(fopen(fname, acc))
            ,_f_size(0) 
        {
            bool success = false;
            _f_size = file_size(fname, success);
            if ( ! success ) {
                _f_size = 0;
//                printf("file_size \"%s\" error=%s\n",fname, strerror(errno));
            }
        }
        ~file_access(){ if (_fp) fclose(_fp); }
        bool fine() const { return _fp != 0; }
        int read(char *buffer, int len) {
            return _fp ? (int)fread(buffer, 1, len, _fp): -1;
        }
        void flush() {
            if ( _fp ) fflush(_fp);
        }
        int write(const char *buf, int len) {
            int ret = -1;
            if ( _fp ) {
                ret = (int)fwrite(buf, 1, len, _fp);
                if ( ret > 0 ) _f_size += ret;
            }
            return ret;
        }
        unsigned long long size() const { return _f_size; }
    };
    class file_rw {
        int _fd;
        long long _f_size;
    public:
        file_rw(const char *fname, int open_mode)
            :_f_size(0) 
        {
            bool success = false;
            _f_size = file_size(fname, success);
            if ( ! success ) {
                _f_size = 0;
                printf("file_size \"%s\" error=%s\n",fname, strerror(errno));
            }
#ifndef _WIN32
            if ( open_mode & O_CREAT ) {
                _fd = open(fname, open_mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            }
            else 
#endif
                _fd = open(fname, open_mode);
        }
        ~file_rw(){ if (_fd != -1 ) close(_fd); }
        bool fine() const { return _fd != -1; }
        int read(char *buffer, int len) {
            if ( fine() ) return (int)::read(_fd, buffer, len);
            return -1;
        }
        void flush() {
        }
        int write(const char *buf, int len) {
            int ret = -1;
            if ( fine() ) {
                ret = (int)::write(_fd, buf, len);
                if ( ret > 0 ) _f_size += ret;
            }
            return ret;
        }
        unsigned long long size() const { return _f_size; }
        bool get_stat(struct stat *file_stat) {
            if ( _fd == -1 ) return false;
            return fstat(_fd, file_stat) == 0;
        }
    };

}
