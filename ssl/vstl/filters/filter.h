#pragma once
namespace filters {
    class filter {
        filter *_next;
    protected:
        virtual filter *next() { return _next; }        
    public:
        filter():_next(0){}
        virtual void connect(filter *next) { _next = next; }
        virtual int read(char *buf, int len) = 0;
        virtual int write(const char *buf, int len) = 0;
        virtual int state(const char *req) = 0;
        virtual void destroy() = 0;
    };
}