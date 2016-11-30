#pragma once
#include <sys/types.h>
#ifdef __IOS_
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#ifndef _NEW_
#define _NEW_
#ifndef _NEW
#define _NEW

#pragma pack(push,8)
#pragma warning(push,3)
struct nothrow_t {};
extern const nothrow_t nothrow;	
//inline void operator delete(void *){}
inline
void * operator new(size_t, void *place) 
{
    return (place);
}
inline
void * operator new[](size_t, void *place) 
{	
    return (place);
}
inline
void * operator new(size_t size) throw()
{ 
    void *ret = malloc(size);
    return ret;
}
inline void  operator delete(void *m, void *p) 
{
    free(m);  	
}
inline void  operator delete[](void *m, void *p) 
{
    free(m);  	
}
inline void operator delete(void *m)
{
    free(m);
}
inline 
void  operator delete[](void *m) 
{
    free(m);
}
inline 
void * operator new[](size_t size) { return malloc(size); }
inline 
void * operator new(size_t size, const nothrow_t&) { return malloc(size); }
inline 
void * operator new[](size_t size, const nothrow_t&) { return malloc(size); }
inline 
void  operator delete(void *m, const nothrow_t&) {free(m);}
inline 
void  operator delete[](void *m, const nothrow_t&) {free(m);}

#pragma warning(pop)
#pragma pack(pop)
#endif /* _NEW */
#endif /* _NEW_ */


