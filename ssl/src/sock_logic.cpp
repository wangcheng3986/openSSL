#include "sock_logic.h"
#include "../util/vstl/L3/protocol/base64_encode.h"

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
    case AF_INET:
#ifndef _WIN32
        inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),s, maxlen);
#endif
        break;

    case AF_INET6:
#ifndef _WIN32
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),s, maxlen);
#endif
        break;

    default:
        strncpy(s, "Unknown AF", maxlen);
        return NULL;
    }

    return s;
}

unsigned short get_port(const struct sockaddr *sa)
{
    if ( sa->sa_family == AF_INET ) {
        const struct sockaddr_in *addr = (const struct sockaddr_in *)sa;
        return ntohs(addr->sin_port);
    }
#ifndef _WIN32
    else if ( sa->sa_family == AF_INET6 ) {
        const struct sockaddr_in6 *addr = (const struct sockaddr_in6 *)sa;
        return ntohs(addr->sin6_port);
    }
#endif
    else {
        return 0;
    }
}

bool get_ip_info(const struct sockaddr *sa, IpAddrInfo &addr)
{
    addr.port = get_port(sa);
    char ip_buf[128] = {0};
    if ( ! get_ip_str(sa, ip_buf, sizeof(ip_buf)) ) return false;
    addr.ip = ip_buf;
    addr.ip_ver = (sa->sa_family == AF_INET6)?6:4;
    return true;
}


int RequestQueue::pre_parse(const char *buf, int len)
{
    _cache.push_back(buf, len);
    char head_buf[8] = {0};
    int head_size = _cache.peek(head_buf, 4);
    if ( head_size < 4 ) return len;

    do {

        if ( strncasecmp(head_buf, "GET", 3) && strncasecmp(head_buf, "POST", 4) && strncasecmp(head_buf, "OPTI", 4) ) 
            break;

        container_l2::my_stream_wrapper<1000> wrapper(_cache);
        protocol3_http::request_decoder req_decoder(0);
        req_decoder.parse(wrapper);
        int head_size = req_decoder.get("head size");
        if ( req_decoder.error() ) 
            break;
        if ( head_size == 0 ) {
            if ( _cache.size() > 1024 * 10 ) 
                break;
            return len;
        }

        if ( req_decoder.query("ver") == 0 ) break;
        if ( strncasecmp(req_decoder.query("ver"), "HTTP/", 5) ) break;

        _state = http_head;
        _cache.pop_back(len);
        return 0;

    } while (false);

    _cache.clear();
    _state = not_http;
    return len;
}

int RequestQueue::parse_head(const char *buf, int len, UINT64 curr_time)
{
//    LOGD("parse head");
//    LOGD("%.*s\n---------------------------\n", len, buf);
    _cache.push_back(buf, len);
    container_l2::my_stream_wrapper<1000> wrapper(_cache);
    protocol3_http::request_decoder req_decoder(0);
    req_decoder.parse(wrapper);

    int head_size = req_decoder.get("head size");
    if ( head_size == 0 ) {
//        LOGD("head_size = 0\n");
        return len;
    }

    const char *content_length = protocol3_http::get_val(req_decoder.params(), "Content-Length");
    if ( content_length == 0 ) {
        _content_left = strncasecmp(req_decoder.query("method"), "GET", 3)? -1: 0;
    }
    else {
        _content_left = atoi(content_length);
    }
    container_l2::smart_buffer<char, 1024 * 10> buffer(head_size + 10);
    _cache.pop_front(head_size, buffer.buffer());
    RequestInfo info;
    if ( _content_left > 0 ) info.upload = _content_left;
    info.request.assign(buffer.buffer(), head_size);
    info.start_time = info.req_end_time = curr_time;
    _reqs.push_back(info);
//    LOGD("_reqs.size() = %d\n", _reqs.size());
    int ret = len - _cache.size();
    _cache.clear();
    _state = http_content;

    return ret;
}

void RequestQueue::push(const char *buf, int len, UINT64 curr_time)
{
    //httpЭ�����
    //����httpЭ��,return
    //��httpЭ��,��request index����, ѹջ, �漰�ϴ���chunk��ʽ����, ����content_len�ķ�chunk��ʽ,��post,����;post, ����һ��request


    while ( len ) {
        int size = 0;
        switch ( _state ) 
        {
        case no_parsed:
            size = pre_parse(buf, len);
            break;
        case http_head:
            size = parse_head(buf, len, curr_time);
            break;
        case http_content:
            if ( _content_left >=0 ) 
            {
                size = unistd_l0::Min<int>(_content_left, len);
                _content_left -= size;
                if ( _content_left == 0 ) _state = http_head;
            }
            else 
            {
                size = len;
            }
            break;
        case not_http:
            size = len;
            break;
        default:
            size = len;
            assert(false);
            break;
        }
        buf += size;
        len -= size;
        _upsize += size;
    }
}

int ResponseQueue::push(const char *buf, int len, UINT64 curr_time)
{
//    printf("push:\n%.*s\n", len, buf);
    int ret = 0;
//    while ( len ) {
        int size = 0;
        switch ( _state ) 
        {
        case http_head:
            if ( response.size() ) return ret;
            size = parse_head(buf, len, curr_time);
//            printf("http_head parsed %d _state = %d\n", size, _state);
            break;
        case http_content:
            if ( ! _chunked ) 
            {
                if ( _content_left >=0 ) 
                {
                    size = unistd_l0::Min<int>(_content_left, len);
                    _content_left -= size;
                    if ( _content_left == 0 ) _state = http_head;
                }
                else 
                {
                    size = len;
                }
            }
            else 
            {
                const char *chunk_buf = buf;
                int chunk_len = len;
                while ( chunk_len ) {

                    //chunked��ʽ
                    dword_t chunk_size = _chunk.push(buf, len);
                    chunk_len -= chunk_size;
                    chunk_buf += chunk_size;
                    size += chunk_size;
                    if ( _chunk.error() ) {
                        _state = protocol_error;
//                        printf("chunk error!!!!\n");
                        break;
                    }
                    if ( _chunk.finished() ) {
//                        printf("chunk finished, chunkcount=%d, responsesize=%lld\n", _chunk.chunk_count(), _chunk.response_size());
                        _content_left = 0;
                        _chunked = false;
                        if (_requests->_reqs.size() ) 
                            _requests->_reqs.front().recved = _chunk.response_size();
                        _chunk.reset();
                        _state = http_head;
                        break;
                    }
                }
            }
            break;
        case protocol_error:
            return ret + len;
            break;
        }
        buf += size;
        len -= size;
        _downsize += size;
        ret += size;
//    }
    return ret;
}

int ResponseQueue::parse_head(const char *buf, int len, UINT64 curr_time)
{

    if ( _requests->_reqs.size() == 0 ) {
        _cache.clear(); return len;
    }
    if ( len && _requests->_reqs.size() ) {

        if ( _requests->_reqs.front().response_time == 0 ) 
            _requests->_reqs.front().response_time = curr_time;
    }
    _cache.push_back(buf, len);
    container_l2::my_stream_wrapper<1000> wrapper(_cache);
    _res.reset();

    int head_size = _res.parse(wrapper);
    if ( head_size == 0 ) return len;

    container_l2::smart_buffer<char, 1024 * 10> buffer(head_size + 10);
    _cache.pop_front(head_size, buffer.buffer());
    response.assign(buffer.buffer(), head_size);
    int ret = len - _cache.size();
    _cache.clear();

    switch ( _res.trans_mode() ) 
    {
    case protocol3_http::response_decoder::chunked: 
//        printf("content encode :chunked\n");
        _chunked = true;
        _content_left = -1;
        break;
    case protocol3_http::response_decoder::byclose:
//        printf("content encode :byclose\n");

        if ( strcasecmp(_res.ver(), "HTTP/1.1") == 0 ) _content_left = 0;
        else _content_left = -1;
        break;
    case protocol3_http::response_decoder::ranged:
    case protocol3_http::response_decoder::fixed:
        _content_left = _res.ret_size();
        break;
    default:
        assert(false);
        break;
    }


    if ( _content_left > -1 ) 
    {
        if ( _requests->_reqs.size() ) 
            _requests->_reqs.front().recved = _content_left;
    }
    _state = _content_left ? http_content: http_head;
//    printf("head parsed, _state = %d\n", _state);
    return ret;

}

void ConnectionInfo::on_up(const char *buf, size_t len)
{
    _requests.push(buf, len, curr_time);
}

void ConnectionInfo::on_down(int fd, const char *buf, size_t len)
{
    if ( ! _requests.is_http() ) {
        _response._downsize += len;
        return;
    }
    int size = 0;
    while ( len ) {
        size = _response.push(buf, len, curr_time);
//         printf("fd(%d)parsed: %d/%d\n",fd, size, len);
//         printf("%.*s", size, buf);
//         printf("\n-----------_req_list.size=%d---------------------\n", _requests._reqs.size());
        buf += size;
        len -= size;
        if ( _response.response.size() ) {

            switch ( _response._state ) 
            {
            case ResponseQueue::http_head:
//                printf("report\n");
                report(fd);
//                printf("report done\n");
                _response.reset();
                break;
            case ResponseQueue::protocol_error:
//                printf("on_user_close\n");
                on_user_close(fd, -4);
//                printf("on_user_close done\n");
                _response.reset();
                return;
                break;
            case ResponseQueue::http_content:
                //δ����
//                printf("not done\n");
                break;
            default:
                assert(false);
                break;
            }
        }
    }
}

void ConnectionInfo::report(int fd)
{
    //���|req��ʼʱ��,req����ʱ�䣬��һ���յ�responseʱ�䣬���һ���յ�responseʱ��|socket id|״̬��|request headers|response headers|send�ֽ���|recv�ֽ���|URL

    container_l2::smart_buffer<char, 1024 * 10> buffer( (_response.response.size() + _requests._reqs.front().request.size() + 1024 ) * 2 );

    protocol3_http::response_decoder & res_decoder = _response._res;
    RequestInfo &req = _requests._reqs.front();
    UINT64 ret_size = 0;
    protocol3_http::request_decoder req_decoder(get_allocator());
    {
        container_l2::stream<1000> req_stream;
        req_stream.push_back(req.request.c_str(), req.request.size());
        container_l2::my_stream_wrapper<1000> wrapper(req_stream);
        req_decoder.parse(wrapper);

        ret_size = req.recved;

        if ( res_decoder.trans_mode() == protocol3_http::response_decoder::byclose )
        {
            ret_size = _response._downsize - _response.response.size();
        }
        else if ( res_decoder.trans_mode() != protocol3_http::response_decoder::chunked )
        {
            ret_size = res_decoder.ret_size() - _response._content_left;
        }
    }
    container_l2::string url(_use_ssl?"https://":"http://", get_allocator());
    url.append(protocol3_http::get_val(req_decoder.params(), "Host"));
	//����uri�д�http ����https���������
	const char *str = req_decoder.query("uri");
	const char *str1 = strstr(str, "http://");
	const char *str2 = strstr(str, "https://");
	if (str1 || str2) {
		url.clear();
	}
	url.append(str);

    int size = sprintf(buffer.buffer(), "REQ|%d:%s|%lld,%lld,%lld,%lld|%d|%d|"
        , 0
        , "success"
        , req.start_time / 1000
        , req.req_end_time / 1000
        , req.response_time / 1000
        , this->curr_time / 1000
        , fd
        , res_decoder.rcode()
        );
    int base64ptr = size;
    size += base64_encode((unsigned const char *)req.request.c_str(), req.request.size(), buffer.buffer() + size);
    size += sprintf(buffer.buffer() + size, "|");
    base64ptr = size;
    size += base64_encode((unsigned const char *)_response.response.c_str(), _response.response.size(), buffer.buffer() + size);
    size += sprintf(buffer.buffer() + size, "|%lld|%lld|%s", req.upload + req.request.size(), ret_size + _response.response.size(), url.c_str());
    _instance->on_task(buffer.buffer());
    _requests._reqs.pop_front();
}

void ConnectionInfo::on_user_close(int fd, int result_code)
{
    //read, ���ӿ϶��������
    if ( ! _requests.is_http() ) return;
    if ( _requests._reqs.size() == 0 ) return ;

    while ( _requests._reqs.size() ) {

        container_l2::smart_buffer<char, 1024 * 20> buffer(_requests._reqs.front().request.size() * 2 + _response.response.size() * 2 + 1024 * 2);

        RequestInfo &req = _requests._reqs.front();
        protocol3_http::request_decoder req_decoder(get_allocator());
        {
            container_l2::stream<1000> req_stream;
            req_stream.push_back(req.request.c_str(), req.request.size());
            container_l2::my_stream_wrapper<1000> wrapper(req_stream);
            req_decoder.parse(wrapper);
        }
		// https or http process
		container_l2::string url("");
		if ( _use_ssl || _ssl )
		{
			url.append("https://");
		}
		else
		{
			url.append("http://");
		}
		
        url.append(protocol3_http::get_val(req_decoder.params(), "Host"));
		//����uri�д�http ����https���������
		const char *str = req_decoder.query("uri");
		const char *str1 = strstr(str, "http://");
		const char *str2 = strstr(str, "https://");
		if (str1 || str2) {
			url.clear();
		}
		url.append(str);

        const char *error_desc = "success";
        switch ( result_code ) 
        {
        case -1: error_desc = "Closed By Local";
            break;
        case -2: error_desc = "Closed By Peer";
            break;
        case -3: error_desc = strerror(_err_num);
            break;
        case -4: error_desc = "chunk parse error";
            break;
        default:
            error_desc = strerror(_err_num);
            break;
        }
        protocol3_http::response_decoder & res_decoder = _response._res;
        if ( res_decoder.parsed() && res_decoder.trans_mode() == protocol3_http::response_decoder::byclose && (result_code == -2 || result_code == -1) ) {
            result_code = 0; error_desc = "success";
        }
        int size = sprintf(buffer.buffer(), "REQ|%d:%s|%lld,%lld,%lld,%lld|%d|"
            , result_code
            , error_desc
            , req.start_time / 1000
            , req.req_end_time / 1000
            , req.response_time / 1000
            , this->curr_time / 1000
            , fd
            );
        if ( res_decoder.parsed() ) {
            size += sprintf(buffer.buffer() + size, "%d", res_decoder.rcode());
        }
        size += sprintf(buffer.buffer() + size, "|");
        size += base64_encode((unsigned const char *)req.request.c_str(), req.request.size(), buffer.buffer() + size);
        size += sprintf(buffer.buffer() + size, "|");
        size += base64_encode((unsigned const char *)_response.response.c_str(), _response.response.size(), buffer.buffer() + size);
        size += sprintf(buffer.buffer() + size, "|%lld|%lld|%s", req.upload + req.request.size(), _response._downsize, url.c_str());

        _instance->on_task(buffer.buffer());
        _requests._reqs.pop_front();
    }
}
