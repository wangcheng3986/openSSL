////
//// Created by Administrator on 2016/12/7.
////
//
//#include "req_decoder.h"
//
//
//dword_t parse(const container_l2::stream_wrapper &stream_data) {
//
//int offset = stream_data.find("\r\n\r\n", 4);
//if ( offset == -1 ) return 0;
//_head_size = offset + 4;
////����ÿһ��
//container_l2::smart_buffer<char, 1024 * 10> buffer(_params.get_allocator(), offset + 5);
//stream_data.peek(buffer.buffer(), _head_size - 2);
//buffer[_head_size - 2] = '\0';
//{
//const char *line_tok = "\r\n";
//int i = 0;
//char *line = buffer.buffer();
//
//for ( char *end ; end = strstr(line, line_tok) ; i++, line = end + 2 ) {
//
//*end = '\0';
//if ( i == 0 ) {
//
//if ( ! this->_parse_protocol(line, end - line) ) {
//_state = 'e';
//_head_size = 0;
//break;
//}
//}
//else if ( ! this->_parse_pragma(line, end - line) ) {
//_state = 'e';
//_head_size = 0;
//break;
//}
//}
//return _head_size;
//}
//
//}
//
//const char *query(const char *qtype) const {
//
//    //"uri", "method", "ver", "state", "count param"
//    switch ( *qtype ) {
//        case 'u':
//            return _uri.c_str();
//        case 'm':
//            return _method[0] ? _method: 0;
//        case 'v':
//            return _ver[0] ? _ver: 0;
//        default:
//            return 0;
//    }
//}
//const char *param(const char *name) const {
//
//    string_map::iterator it = _params.find(string( name, _params.get_allocator() ));
//    return ( it == _params.end() ) ? 0: it->second.c_str();
//}
//string_map &params() { return _params; }
//
//dword_t get(const char *qtype) const {
//
//    //"head size", "parameter count",
//    switch ( *qtype ) {
//        case 'h':
//            return _head_size;
//        default:
//            return 0;
//    }
//}
//void reset() {
//
//    _uri.clear();
//    _params.clear();
//    _method[0] = _ver[0] = '\0';
//    _state = 'f';
//    _head_size = 0;
//}
//
//class iterator {
//    string_map::iterator _it;
//    public:
//    iterator(string_map::iterator it):_it(it){}
//    iterator(const iterator &right) :_it(right._it){}
//    iterator &operator = (const iterator &right) { _it = right._it; return *this; }
//    iterator &operator ++ () { ++_it;  return *this; }
//    iterator &operator -- () { --_it;  return *this; }
//    string_map::map_node &operator * () {  return *_it; }
//    string_map::map_node &operator -> () { return *_it; }
//    bool operator == (const iterator &right) { return _it == right._it; }
//    bool operator != (const iterator &right) { return ! operator == (right); }
//};
//iterator end() const { return iterator(_params.end()); }
//iterator begin() { return iterator(_params.begin());}
//iterator find(const string &value) {
//return iterator(_params.find(value));
//}