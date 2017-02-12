#ifndef _AIDSOCK_H_
#define _AIDSOCK_H_

#include <pthread.h>
#include "../../common/crc8.h"
#include "../../common/config.h"
#include "../../common/utility.h"
#include "../../common/cJSON.h"

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
