//
// Created by Administrator on 2016/12/7.
//

#ifndef LOGIC_CHUNK_H
#define LOGIC_CHUNK_H


//void reset() {
//    _response_size = _chunk_size = _cache_size = _chunk_left = _chunk_count = 0;
//    _state = chunk_head;
//}
//int chunk_count() const { return _chunk_count; }
//bool finished() const { return _state == chunk_done;}
//bool error() const { return _state == chunk_error; }
//qword_t response_size() const { return _response_size; }
//int push(const char *buffer, int len) {
//
//    int in_len = len;
//    while ( len ) {
//        int size = 0;
//        bool parsed = false;
//        switch ( _state )
//        {
//            case chunk_head:
//                size = parse_head(buffer, len, parsed);//����_chunk_size �� _chunk_left
//                if ( error() || ! parsed ) return in_len - len;
//                break;
//            case chunk_content:
//                size = unistd_l0::Min<int>(len, _chunk_left);
//                _chunk_left -= size;
//                _response_size += size;
//                if ( _chunk_left == 0 ) _state = chunk_content_end;
//                break;
//            case chunk_content_end:
//                size = parse_end(buffer, len);
//                if ( error() ) return in_len - len;
//                break;
//            case chunk_done:
//                return in_len - len;
//            case chunk_error:
//                return in_len - len;
//            default:
//                assert(false);
//                break;
//        }
//        buffer += size;
//        len -= size;
//    }
//    return in_len - len;
//}
//int parse_end(const char *buffer, int len) {
//    assert(_cache_size < 2);
//    if ( len + _cache_size < 2 ) {
//        if ( len ) {
//            _chunk_cache[0] = buffer[0];
//            _cache_size += len;
//        }
//        return len;
//    }
//    int ret = 2 - _cache_size;
//    memcpy(_chunk_cache + _cache_size, buffer, ret);
//
//    if ( strncmp(_chunk_cache, "\r\n", 2) ) {
//        _state = chunk_error;
//        return 0;
//    }
//    _cache_size = 0;
//    _state = _chunk_size ? chunk_head: chunk_done;
//    return ret;
//}
//int parse_head(const char *buffer, int len, bool &b_parsed) {
//
//    b_parsed = false;
//    int ret = sizeof(_chunk_cache) - 1 - _cache_size;
//
//    ret = unistd_l0::Min(ret, len);
//    memcpy(_chunk_cache + _cache_size, buffer, ret);
//    _chunk_cache[_cache_size + ret] = '\0';
//    if ( ret + _cache_size <= 2 ) {
//        _cache_size += ret;
//        return ret;
//    }
//    char *end_tok = strstr(_chunk_cache, "\r\n");
//    if ( end_tok == 0 ) {
//        if ( _cache_size + ret == sizeof(_chunk_cache) - 1 ) {
//            _state = chunk_error;
//        }
//        _cache_size += ret;
//        return ret;
//    }
//    ret = (int)(end_tok + 2 - _chunk_cache - _cache_size);
//    *end_tok = '\0';
//    _chunk_size = _chunk_left = unistd_l0::atox<type_l0::dword_t>(_chunk_cache);
//    _chunk_count++;
//    _state = chunk_content;
//    _cache_size = 0;
//    b_parsed = true;
//    return ret;
//}
#endif //LOGIC_CHUNK_H
