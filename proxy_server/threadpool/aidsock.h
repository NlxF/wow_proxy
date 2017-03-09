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
void *read_sock(void *p);

/**
  说明：写sock
  参数：IN：SOCKDATA结构指针，包含发送到tserver的命令, OUT:tserver返回的内容
  返回值：无
  **/
void *write_sock(void *p);

#endif
