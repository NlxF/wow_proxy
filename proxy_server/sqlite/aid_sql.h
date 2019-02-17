#ifndef _AID_SQL_H_
#define _AID_SQL_H_

#include "sqlite3.h"
#include "../../common/config_.h"

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


/**
 * 初始化请求授权
 * @return 成功返回0，失败返回非零
*/
int init_request_auth();


/**
 * 获取请求授权
 * @return 成功返回缓存的指针，失败返回NULL
*/
char *fetch_request_auth();


/**
 释放请求授权缓存
 */
void destory_request_auth();

#endif
