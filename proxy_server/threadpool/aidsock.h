#ifndef _AIDSOCK_H_
#define _AIDSOCK_H_

#include <pthread.h>
#include "../../common/crc8.h"
#include "../../common/config_.h"
#include "../../common/utility.h"
#include "../../common/cJSON.h"
#include "../sqlite/aid_sql.h"


typedef enum
{
    DEPRECATED = -1,     //命令已废弃
    PARAMNUMERR = -2,    //命令需要的参数个数不符
    PARAMTYPEERR = -3,   //类型不正确
    COMMANDNOEXIST = -4, //当前命令不存在
    PARAMNULL = -5,      //参数为空
    PARAMORDERERR = -6   //参数顺序不正确
    
} JSONERROR;


/**
  说明：读取sock的字节流
  参数：IN:SOCKDATA结构指针，包含发送到wserver的命令, OUT:wserver返回的内容
  返回值：无
  **/
void *read_sock_func(void *p);

/**
  说明：写sock
  参数：IN：SOCKDATA结构指针，包含发送到tserver的命令, OUT:tserver返回的内容
  返回值：无
  **/
void *write_sock_func(void *p);


/**
 初始化用于与soap服务通信的socket table
 @param pthreads 线程指针，用作table的key
 @param thread_num 线程数量
 @return table ptr
 */
//void* init_soap_socket_table(pthread_t* pthreads, int thread_num);

/**
 查找hash table中key值对应的value
 @param t 创建的socket table
 @param key key值
 @return sock值
 */
//int socket_for_key(table t, int key);

#endif
