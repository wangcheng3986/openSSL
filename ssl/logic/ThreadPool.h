//
// Created by Administrator on 2016/12/5.
//

#ifndef LOGIC_THREADPOOL_H_H
#define LOGIC_THREADPOOL_H_H


#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <stddef.h>


/*
*线程池里所有运行和等待的任务都是一个CThread_worker
*由于所有任务都在链表里，所以是一个链表结构
*/
typedef struct worker
{
    /*回调函数，任务运行时会调用此函数，注意也可声明成其它形式*/
    void *(*process) (void *arg, int mode, long id);
    void *arg;/*回调函数的参数*/
    int mode;/*回调函数的参数*/
    struct worker *next;

} CThreadworker;



/*线程池结构*/
typedef struct
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;

    /*链表结构，线程池中所有等待任务*/
    CThreadworker *queue_head;

    /*是否销毁线程池*/
    int shutdown;
    pthread_t *threadid;
    /*线程池中允许的活动线程数目*/
    int max_thread_num;
    /*当前等待队列的任务数目*/
    int cur_queue_size;

} CThreadpool;


void pool_init ();
/*向线程池中加入任务*/
int  pool_add_worker (void *(*process) (void *, int ), void *arg, int mode);

int pool_destroy ();

#ifdef  __cplusplus
}
#endif

#endif //LOGIC_THREADPOOL_H_H
