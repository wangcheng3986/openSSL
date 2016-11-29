#pragma once
#include "../../L0/abstract/allocator.h"
#include "../../L0/abstract/type.h"
#include "../../L2/buffer/stream.h"
#include "../../L2/buffer/buffer.h"
#include "../../L2/containers/map.h"
#include "../../L2/containers/string.h"
namespace configure_l3 {

    inline bool isSpace(unsigned char ch) {
        return (unsigned char)(ch - 9) < 5 || ch == ' ';
    }
    inline char *skip_spec(char *buf)
    {
        while ( *buf && isSpace(*buf) ) buf++;
        return buf;
    }
    inline char *skip_rspec(char *buf)
    {
        char *end = buf + strlen(buf) - 1;
        while ( (end > buf) && isSpace(*end) ) end--;
        if ( ! isSpace(*end) ) end++;
        return end;
    }
    inline char *word_end(char *buf)
    {
        while ( *buf && ! isSpace(*buf) ) buf++;
        return buf;
    }

    class Configure {
    public:
        typedef container_l2::string string;
        typedef container_l2::string::my_allocator string_allocator;
        typedef container_l2::map<string, string> StringMap;
        typedef container_l2::map<string, StringMap> SessionMap;
        SessionMap _sessions;
    private:
        StringMap _notes;
        typedef container_l2::stream<1000> MyStream;
        MyStream _cache;
        StringMap *_top_session;
        bool _no_session;

        bool parse_session(char *line) {

            if ( line[0] != '[' ) return false;
            line = skip_spec(line + 1);
            char *end = strrchr(line, ']');
            if ( end == 0 ) return false;
            *end = '\0';
            *skip_rspec(line) = '\0';

            _top_session = &(_sessions[string(line, _sessions.get_allocator())]);
            if ( _top_session->get_allocator() == 0 ) _top_session->set_allocator(_sessions.get_allocator());
            return true;
        }
        void parse_line(char *line) {

            line = skip_spec(line);
            if ( *line == '\0' ) return;//¿ÕÐÐ

            //×¢ÊÍÐÐ
            for ( StringMap::iterator it = _notes.begin(); it != _notes.end(); ++it ) {
                if ( strstr(line, it->first.c_str()) == line ) return;
            }
            if (parse_session(line)) return;

            char *token = strchr(line, '=');
            char *value = (char*)"";
            if ( token ) {

                *token = '\0';
                *skip_rspec(line) = '\0';
                value = skip_spec(token + 1);
                *skip_rspec(value) = '\0';
            }
            else *skip_rspec(line) = '\0';
            if ( _top_session == 0 ) {
                _top_session = &(_sessions[""]);
                if ( _top_session->get_allocator() == 0 ) _top_session->set_allocator(_sessions.get_allocator());
            }
            (*_top_session)[string(line, _sessions.get_allocator())] = value;
            if ( _no_session ) {

                StringMap &empty_session = _sessions[""];
                if ( empty_session.get_allocator() == 0 ) empty_session.set_allocator(_sessions.get_allocator());
                if ( &empty_session != _top_session ) {
                    empty_session[string(line, _sessions.get_allocator())] = value;
                }
            }
        }


        void parse() {

            MyStream::iterator end = _cache.end();
            
            int offset;
            container_l2::my_stream_wrapper<1000> wrapper(_cache);
            while ( (offset = wrapper.find("\n", 1)) != -1 ) {

                container_l2::smart_buffer<char, 1024 * 10> buffer(_sessions.get_allocator(), offset + 1);
                _cache.pop_front(offset + 1, buffer.buffer());
                buffer[offset] = '\0';
                parse_line(buffer.buffer());
            }
        }
    public:
        Configure(allocator_l0::allocator *Allocate, bool no_session=false) 
            : _cache(Allocate)
            , _sessions(Allocate)
            , _top_session(0)
            , _notes(Allocate)
            , _no_session(no_session){
            note_token("#");
        }
        void note_token(const char *tokens) { _notes[string(tokens, _sessions.get_allocator())]; }
        void push( const char *buf, type_l0::dword_t len ) {
            _cache.push_back(buf, len);
            parse();
        }
        void commit() {
            push("\n", 1);
            _top_session = 0;
        }
    };
}