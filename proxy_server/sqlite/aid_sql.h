#ifndef _AID_SQL_H_
#define _AID_SQL_H_

#include "sqlite3.h"
#include "../../common/config_.h"
#include "../../common/hashtable/hashtable.h"


/**
 初始化命令表
 @return 成功返回0，失败返回非零
 */
int init_commands_table();


/**
 根据key返回value
 @param key 所求value的keu
 @return value值
 */
Command *value_for_key(int key);


/**
 释放命令表
 */
void destory_commands_table();


#endif
