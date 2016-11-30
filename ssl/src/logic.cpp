#include "../logic.h"
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <jni.h>
#include "../util/vstl/L3/system/time.h"
#include "C_sockethandler.h"

//#define JNIREG_CLASS "com/nbs/api/SSLListener"
//extern JavaVM * gVm;
//static void onMessage(const char *log);
//static void onMessage(const char *log){
//    if ( gVm == 0 ) return;
//    JNIEnv *env = NULL;
//
//    if ( gVm->AttachCurrentThread(&env, NULL) < 0 ) return;
//    jclass javaProvider = env->FindClass(JNIREG_CLASS);
//    if( ! javaProvider ) {
//        return;
//    }
//    jmethodID method = env->GetStaticMethodID(javaProvider, "onMessage","(Ljava/lang/String;)V");
//    if(!method) {
//        return;
//    }
//    env->CallStaticVoidMethod(javaProvider, method, env->NewStringUTF(log) );
//}



UINT64 nb_getSysTime(){
//    LOGD("CCCCCCCCCCCCCCCCC");
//    onMessage("cc:getSysTime");
//    LOGD("DDDDDDDDDDDDDDDDDDD");
    return system_time3::GetTickCount64();
}

void nb_ssl_create(void *ctx, void *ret, UINT64 start_time, UINT64 end_time){
//    LOGD("nb_ssl_create");
    get_ssl_handler()->on_ssl_create(ctx, ret, start_time, end_time);
}
void nb_ssl_close(void *ssl, UINT64  start_time){
//    LOGD("nb_ssl_close");
    get_ssl_handler()->on_ssl_close(ssl, start_time);
}
void nb_ssl_connect(void *ssl, int ret, UINT64 start_time, UINT64 end_time, int fd){
//    LOGD("nb_ssl_connect");
    get_ssl_handler()->on_ssl_connect(ssl,ret,start_time,end_time,fd);
}

void nb_ssl_read(void *ssl,void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
//    LOGD("nb_ssl_read");
    get_ssl_handler()->on_ssl_read(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_write(void *ssl,const void *buf,int num, int ret, UINT64 start_time, UINT64 end_time){
//    LOGD("nb_ssl_write");
    get_ssl_handler()->on_ssl_write(ssl, buf, num, ret, start_time, end_time);
}
void nb_ssl_set_fd(void *s, int fd, int ret, UINT64 start_time){
//    LOGD("nb_ssl_set_fd");
    get_ssl_handler()->on_ssl_set_fd(s, fd, ret, start_time);
}