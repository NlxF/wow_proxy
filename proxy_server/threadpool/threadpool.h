/********************************************************
一般一个简单线程池至少包含下列几个部分：
1.线程池管理器（threadpoolmanager）:用于创建并管理线程池
2.工作线程（workthread）：线程池中线程
3.任务接口（task）：每个任务必须实现的接口，以供工作线程调度任务的执行
4.任务队列：用于存放没有处理的任务。提供一种缓冲机制
******************************************************/

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "global.h"

/**
  定义任务节点
**/
typedef void*(*FUNC)(void*arg);

typedef struct _threadpool_job_node
{
	FUNC                          function;     //函数指针
	void                         *arg;          //函数参数
	struct _threadpool_job_node  *prev;         //指向上一个节点
	struct _threadpool_job_node  *next;         //指向下一个节点
}threadpool_job_node;

/**
  定义工作队列
**/
typedef struct _threadpool_job_queue
{
	int                           job_num;      //任务数
	sem_t                        *queue_sem;    //x信号量
	threadpool_job_node          *head;         //队列头指针
	threadpool_job_node          *tail;         //队列尾指针
}threadpool_job_queue;

/**
  线程池
**/
typedef struct _threadpool
{
	int                        threads_num;      //线程数
	pthread_t                 *pthreads;         //线程指针数
	threadpool_job_queue      *job_queue;        //指向队列指针
}threadpool;

typedef struct _thread_data
{
	pthread_mutex_t   *pmutex;
	threadpool       *pthread;
}thread_data;


/**
说明：返回CPU可用数
参数：无
返回值：可用的CPU数，或1;
**/
int get_cpu_num();

/**
 说明：初始化线程池
 参数：线程数
 返回值：线程池指针
 **/
threadpool *threadpool_init(int thread_num);

/**
 说明：线程函数，通过此函数调用正真的task,
 参数：线程池指针
 返回值：无
 **/
void threadpool_thread_run(threadpool* pthreadpool);

/**
 说明：将消息加入线程池
 参数：1.线程池指针
	   2.工作函数
	   3.传入工作函数的参数
 返回值：成功返回0
 **/
int threadpool_add_work(threadpool *pthpool, FUNC pfun, void* argv);



/**
 说明：销毁线程池
 参数：线程池指针
 返回值：无
 **/
void threadpool_destroy(threadpool* pthpool);

/**
 说明：初始化工作队列
 参数：线程池指针
 返回值：成功返回0，失败返回-1
 **/
int threadpool_job_queue_init(threadpool *pthpool);

/**
 说明：添加工作节点到工作队列中
 参数：1.线程池指针
	   2.新的工作节点
 返回值：无
 **/
void threadpool_job_queue_add(threadpool *pthpool, threadpool_job_node *pnew_job);

/**
 说明：移除工作队列最前的节点
 参数：线程池指针
 返回值：成功返回0，失败返回-1
 **/
threadpool_job_node *threadpool_job_queue_remove_first(threadpool *pthpool);

/**
 说明：遍历工作队列 返回最后节点
 参数：线程池指针
 返回值：节点指针
 **/
//threadpool_job_node *threadpool_job_queue_peek(threadpool *pthpool);

/**
 说明：清空工作队列
 参数：线程池指针
 返回值：无
 **/
void threadpool_job_queue_empty(threadpool *pthpool);


#endif
