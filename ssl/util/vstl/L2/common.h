#pragma once
#include "../L0/abstract/allocator.h"
#include "../L0/abstract/type.h"
namespace container_l2 {
    using namespace type_l0;
    template <word_t node_size> class stream;
    template <class T> class list;
    template <class T, class Y> class map;
    template <class Elem, int raw_size> class base_string;
    template <typename T, bool fast> class vector;
}
namespace protocol3_http {

    class request_encoder;
    class request_decoder;
    class response_encoder;
    class response_decoder;
}
// template <>
// static void set_allocator(protocol3_http::request_encoder &val, allocator_l0::allocator *Allocate) 
// {
//     val.set_allocator(Allocate);
// }
// 
// template <>
// static void set_allocator(protocol3_http::request_decoder &val, allocator_l0::allocator *Allocate) 
// {
//     val.set_allocator(Allocate);
// }
// 
// template <>
// static void set_allocator(protocol3_http::response_encoder &val, allocator_l0::allocator *Allocate) 
// {
//     val.set_allocator(Allocate);
// }
// 
// template <>
// static void set_allocator(protocol3_http::response_decoder &val, allocator_l0::allocator *Allocate) 
// {
//     val.set_allocator(Allocate);
// }


template<class T, bool fast>
static void set_allocator(container_l2::vector<T, fast> &val, allocator_l0::allocator *Allocate) 
{
    val.set_allocator(Allocate);
}

template<class T, int raw_size>
static void set_allocator(container_l2::base_string<T, raw_size> &val, allocator_l0::allocator *Allocate) 
{
    val.set_allocator(Allocate);
//    printf("set_allocator(string, %p\n",Allocate);
}

template<class T, class Y>
static void set_allocator(container_l2::map<T, Y> &val, allocator_l0::allocator *Allocate) {
    val.set_allocator(Allocate);
}

template<class T>
static void set_allocator(container_l2::list<T> &val, allocator_l0::allocator *Allocate) {
    val.set_allocator(Allocate);
}

template <type_l0::word_t node_size>
static void set_allocator(container_l2::stream<node_size> &val, allocator_l0::allocator *Allocate) 
{
    val.set_allocator(Allocate);
}
