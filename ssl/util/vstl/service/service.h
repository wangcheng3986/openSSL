#pragma once
#include "../L0/containers/string.h"
#include "../L1/protocol/json.h"
#include "../L2/containers/string.h"
#include "../L2/buffer/stream.h"
#include "../L2/containers/map.h"
#include "../L3/system/thread.h"
#include "../L3/io/sock_io.h"
#include "../L3/protocol/conf.h"
#include "../L3/io/file.h"
#include "../log/log_creater.h"
#include "filter.h"

using namespace type_l0;
template <class T> struct dll_module;

template <class T>
static 
void set_allocator(dll_module<T> &val, allocator_l0::allocator *Allocate) 
{
    val.set_allocator(Allocate);
}

typedef container_l2::string string;
template <class T>
struct dll_module {
    dll_module(allocator_l0::allocator *Allocator)
        : _module(0)
        , _use(0)
        , _module_path(Allocator)
        , _config_file(Allocator)
        , _proc_name(Allocator)
    {}
    dll_module()
        : _module(0)
        , _use(0)
    {}
    void set_allocator(allocator_l0::allocator *Allocator) {

        _module_path.set_allocator(Allocator);
        _config_file.set_allocator(Allocator);
        _proc_name.set_allocator(Allocator);
    }
    ~dll_module() { clean(); }
    T *_module;
    system_dll3::dll_loader _loader;
    type_l0::dword_t _use;
    string _module_path;
    string _config_file;
    string _proc_name;
    void inc() { _use++; }
    void dec() { _use--; }
    type_l0::dword_t used() const { return _use; }

    void clean() {

        if ( _module ) { _module->destroy(); _module = 0; }
        _loader.unload();
    }
    bool load(helper *sys, T *next) {

        do {
            if ( ! _loader.load(_module_path.c_str()) ) break;
            typedef T *(*CreateProc)(helper *sys, const char *, T *);
            void *proc_ptr = _loader.query_proc(_proc_name.c_str());
            if ( proc_ptr == 0 ) break;
            CreateProc fn_create = (CreateProc) proc_ptr;
            _module = fn_create(sys, _config_file.c_str(), next);
            return _module != 0;
        } while (false);
        _loader.unload();
        return false;
    }
    void unload() { _loader.unload(); }

    void init(allocator_l0::allocator *Allocate, const char *module_path, const char *config_file, const char *proc_name) {
        set_allocator(Allocate);
        _module_path.assign(module_path);
        _config_file.assign(config_file);
        _proc_name.assign(proc_name);
    }
};


class frame_conn : public fconnection {
public:
    frame_conn(network_l3::connection *obj):_obj(obj), _droped(false){}
    ~frame_conn(){}
    network_l3::connection *_obj;
    bool _droped;
    virtual void destroy() { _droped = true; }
    virtual const char *state(const char *state_type) { return _obj->state(state_type); }
    //"read"  => "empty", "ready"; 
    //"write" => "ready", "full";
    //"error" => "breaked", "fine";
    virtual dword_t write(const char *data, dword_t len) { return _obj->write(data, len); }
    virtual dword_t read(char *buf, dword_t len) { return _obj->read(buf, len); }
};
class filter_frame :public thread_l3::service_base, public helper{
    allocator_l0::allocator *_allocate;
    network_l3::tcp_sock_poll *_tcp_inst;
    network_l3::listenner *_listenner;
    log_writer::log_creater *_log_creater;
    log_writer::log_writer *_log_file;
    virtual void *query_interface(const char *interface_name) {
        if ( strcmp(interface_name, "allocator") == 0 ) return _allocate;
        if ( strcmp(interface_name, "log_creater") == 0 ) return _log_creater;
        return 0;
    }
    virtual network_l3::tcp_sock_poll *create_socks() {
        return network_l3::tcp_sock_poll::create(_allocate);
    }

    typedef container_l2::map<network_l3::sock_obj *, frame_conn *> _CONN_MAP;
    _CONN_MAP _conns;
    _CONN_MAP _ready_conns;
    typedef container_l2::string string;
    string _log_path;
    string _module_root;
    typedef container_l2::map<string, string> StringMap;
    typedef container_l2::map<int, dll_module<ifilter> > _MODULES_MAP;
    _MODULES_MAP _modules;

    virtual bool process() {

        bool ret = _log_creater->loop_check();

        network_l3::sock_obj *evt_list[1000];
        type_l0::dword_t evt_count = _tcp_inst->query(evt_list, 1000);

        for ( type_l0::dword_t i = 0; i < evt_count; i++ ) {

            ret = parse_evt(evt_list[i]) ? true: ret;
        }
        for ( _MODULES_MAP::iterator it = _modules.begin(); it != _modules.end(); ++it ) {
            ifilter *filter_obj = it->second._module;
            filter_obj->loop_check();
        }
        return ret;
    }


    frame_conn *create(network_l3::connection *obj) {
        frame_conn *ret = static_cast<frame_conn*>( _allocate->allocate(sizeof(frame_conn)) );
        new ((void*)ret) frame_conn(obj);
        return ret;
    }
    void destroy(frame_conn *conn) {
        conn->~frame_conn();
        _allocate->deallocate(conn);
    }
private:
    bool parse_evt(network_l3::sock_obj *obj) {

        switch ( *(obj->type()) ) {
        case 'l': 
            return parse_listen(static_cast<network_l3::listenner *>(obj));
        case 'c': 
            return parse_data(static_cast<network_l3::connection *>(obj));
        default:
            assert(false);
            break;
        }
        return false;
    }
    bool parse_listen(network_l3::listenner *listenner) {

        bool ret = false;
        while ( network_l3::connection *conn = listenner->accept() ) {
            assert ( _ready_conns.find(conn) == 0 );
            _ready_conns[conn] = create(conn);
            ret = true;
        }
        return ret;
    }
    bool parse_data(network_l3::connection *obj) {
        frame_conn *conn = 0;
        _CONN_MAP::iterator it = _conns.find(obj);
        if ( it != _conns.end() ) conn = it->second;
        else {
            it = _ready_conns.find(obj); conn = it->second;
            _ready_conns.erase(it);
            _conns[obj] = conn;
        }
        {
            _MODULES_MAP::iterator it = _modules.begin();
            ifilter *first_filter = it->second._module;
            first_filter->on_event(conn);

            if ( conn->_droped ) {

                _conns.erase(obj);
                obj->destroy();  destroy(conn);
            }
        }
        return true;
    }
    void close_conn(network_l3::connection *conn) {
        _CONN_MAP::iterator it = _ready_conns.find(conn);
        if ( it != _ready_conns.end() ) {
            conn->destroy();
            destroy(it->second);
            _ready_conns.erase(it);
        }
        else {
            it = _conns.find(conn);
            if ( it != _conns.end() ) {
                conn->destroy();
                destroy(it->second);
                _conns.erase(it);
            }
        }
    }

    bool init(const char *configfile) {

        //解析json配置文件
        storage_l3::file_access fa(configfile, "rb");
        char buffer[1024 * 100];
        if ( fa.size() >= sizeof(buffer) ) {
            printf("too large json config file\n"); return false;
        }
        int size = fa.read(buffer, sizeof(buffer) - 1);
        if ( size <= 0 ) return false;
        buffer[size] = '\0';

        protocol1_json::jvariant data(buffer);
        if ( data._type != 'o' ) return false;
        protocol1_json::jobject &json_object = reinterpret_cast<protocol1_json::jobject &> (data);
        int i = 0;
        int listen_port = -1;
        for ( protocol1_json::jobject::iterator it = json_object.begin(); it != json_object.end(); ++it, i++) {

            protocol1_json::jobject::paire &object = *it;
            if ( object._nlen == 0 ) continue;
            protocol1_json::jvariant filter_object(object._value);
            if ( filter_object._type != 'o' ) continue;
            protocol1_json::jobject &obj_info = reinterpret_cast<protocol1_json::jobject &> (filter_object);

            container_l0::const_string filter_name;
            container_l0::const_string filter_path;
            container_l0::const_string config_path;
            container_l0::const_string obj_type;
            container_l0::const_string proc_name("create_inst", 11);
            double level = 0.0;
            int value_int = -1;
            for ( protocol1_json::jobject::iterator it = obj_info.begin(); it != obj_info.end(); ++it, i++ ) {
                container_l0::const_string name(it->_name, it->_nlen);
                if ( name == "type" ) {
                    protocol1_json::jvariant type_val(it->_value);
                    if ( ! protocol1_json::read_string(type_val, obj_type) ) continue;
                }
                else if ( name == "level" ) {
                    protocol1_json::jvariant level_val(it->_value);
                    if ( ! protocol1_json::read_value(level_val, level) ) continue;
                    value_int = (int)level;
                }
                else if ( name == "value" ) {
                    protocol1_json::jvariant value_val(it->_value);
                    if ( ! protocol1_json::read_value(value_val, level) ) continue;
                    value_int = (int)level;
                }
                else if ( name == "name" ) {
                    protocol1_json::jvariant tmp_val(it->_value);
                    if ( ! protocol1_json::read_string(tmp_val, filter_name) ) continue;
                }
                else if ( name == "path" ) {
                    protocol1_json::jvariant tmp_val(it->_value);
                    if ( ! protocol1_json::read_string(tmp_val, filter_path) ) continue;
                }
                else if ( name == "conf" ) {
                    protocol1_json::jvariant tmp_val(it->_value);
                    if ( ! protocol1_json::read_string(tmp_val, config_path) ) continue;
                }
                else if ( name == "creater" ) {
                    protocol1_json::jvariant tmp_val(it->_value);
                    if ( ! protocol1_json::read_string(tmp_val, proc_name) ) continue;
                }
            }

            if ( obj_type == "filter" ) {

                if ( value_int == -1 ) continue;
                if ( filter_path.len() == 0 ) continue;
                if ( _modules.find(value_int) != _modules.end() ) continue;
                string path(_allocate); path.assign(filter_path.c_str(), filter_path.len());
                string conf(_allocate); conf.assign(config_path.c_str(), config_path.len());
                string proc(_allocate); proc.assign(proc_name.c_str(), proc_name.len());
                _modules[value_int].init(_allocate, path.c_str(), conf.c_str(), proc.c_str());
            }
            else if ( obj_type == "port" ) {
                listen_port = value_int;
            }
            else if ( obj_type == "log" ) {
                _log_path.assign(filter_path.c_str(), filter_path.len());
            }
            else if ( obj_type == "moduleroot" ) {
                _module_root.assign(filter_path.c_str(), filter_path.len());
            }
        }
        _log_file = _log_creater->create(_log_path.c_str());
        if ( _modules.size() == 0 ) {
            _log_file->printf(log_writer::log_error, "no filter found\n"); return false;
        }


        if ( listen_port == -1 ) {
            _log_file->printf(log_writer::log_error, "listen port not defined\n");
            return false;
        }
        int id = 0;
        for ( _MODULES_MAP::iterator it = _modules.begin(); it != _modules.end(); ++it, ++id ) {

            if ( it->first != id ) {
                _log_file->printf(log_writer::log_error, "filter %d not found\n", id);
                return false;
            }
        }
        //加载filter

        ifilter *next = 0;
        while ( id > 0 ) {
            --id;
            if ( ! _modules[id].load(this, next) ) {
                _log_file->printf(log_writer::log_error, "filter %d %s load failed\n", id, _modules[id]._module_path.c_str());
                return false;
            }
            next = _modules[id]._module;
        }

        //listen socket
        //
        network_l3::_SERVICE_ADDR addr; addr.port = listen_port;
        char ip_addr[32];
        _listenner = _tcp_inst->listen(addr.to_string(ip_addr));
        if ( _listenner == 0 ) {
            if ( _log_file ) _log_file->printf(log_writer::log_error, "listen(%s) failed\n", ip_addr);
        }
        else {
            if ( _log_file ) _log_file->printf(log_writer::log_info, "filter frame start @ %s\n", ip_addr) ;
        }
        return _listenner != 0;
    }
public:
    virtual bool running() { return _listenner != 0; }

    int count() const { return _conns.size() + _ready_conns.size(); }
    filter_frame(allocator_l0::allocator *Allocate, const char *config)
        : _allocate(Allocate)
        , _conns(Allocate)
        , _ready_conns(Allocate)
        , _modules(Allocate)
        , _module_root(Allocate)
        , _log_path(Allocate)
        , _listenner(0)
        , _log_creater(log_writer::log_creater::create(Allocate))
        , _log_file(0)
    {
        _tcp_inst = network_l3::tcp_sock_poll::create(Allocate);
        init(config);
    }
    ~filter_frame() {

        if ( _log_file ) {
            _log_file->printf(log_writer::log_info, "filter frame stoped\n");
        }

        while ( _modules.size() ) {
            _MODULES_MAP::iterator it = _modules.begin();
            it->second.clean();
            _modules.erase(it);
        }
        while ( _ready_conns.size() ) {
            _CONN_MAP::iterator it = _ready_conns.begin();
            close_conn(static_cast<network_l3::connection *>(it->first));
        }
        while ( _conns.size() ) {
            _CONN_MAP::iterator it = _conns.begin();
            close_conn(static_cast<network_l3::connection *>(it->first));
        }
        if ( _listenner ) _listenner->destroy();
        if ( _tcp_inst ) _tcp_inst->destroy();
        if ( _log_file ) {

            while ( ! _log_file->finished() ) {
                _log_creater->loop_check(); Sleep(1);
            }
            _log_file->destroy();
        }
        while ( ! _log_creater->try_destroy() ) Sleep(1);
    }
};
