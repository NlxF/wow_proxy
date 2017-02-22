#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <signal.h>
#include "config_.h"

/**
说明：将当前进程转化为守护进程
参数：nochdir:0为将当前目录切换到根目录（需要root权限),非0不切换
     noclose:0为将stdin,stdout,stderr重定向到/dev/null, 非0不重定向
返回值：o-成功，非0-失败
**/
int create_daemon(int nochdir, int noclose);

/**
说明：初始化子进程，无返回值，出错直接退出进程
参数：服务进程名
**/
void inline init_child_image(char* servername);

/**
说明：获取绝对路劲
参数：返回获取的路径，不包含程序名,不带最后的/
返回值：0-成功，非0-失败
**/
int get_absolute_path(char *current_absolute_path);

/**
说明：设置进程的最大fd
参数：如果为0 则为RLIM_INFINITY
**/
void set_process_max_fd(int max);

/**
 创建两条命名管道
 @param write_fd 返回写管道
 @param read_fd  返回读管道
 @return 是否创建成功
 */
bool redirect_namedpip(int *write_fd, int *read_fd);

/**
说明：默认在端口8083,开启监听服务
参数：端口号
返回值：成功返回监听套接字，失败返回<0
**/
int create_server(int port);

/**
说明：设置fd为非阻塞模式
参数：文件描述符
返回值：成功返回0,失败返回<0
**/
int set_non_blocking(int fd);

/**
说明：创建epoll
参数：文件描述符
返回值：成功返回epoll文件描述符，失败返回值<0
**/
int create_epoll(int sock_fd);

/**
 添加监听sock添加到epoll集合
 @param sockConn SOCKCONN指针
 */
void add_epoll_fd(SOCKCONN *sockConn);

/**
 更新已添加到epoll轮询中的sock的关注事件
 @param sockConn SOCKCONN指针
 */
void modify_epoll_fd(SOCKCONN *sockConn);

/**
 创建SOCKDATA
 @param sock_fd  client socket
 @param read_fd  读管道fd
 @param write_fd 写管道fd
 @return SOCKDATA指针
 */
SOCKDATA *malloc_sockData(SOCKCONN *sockConn, int read_fd, int write_fd);

/**
 释放创建的SOCKDATA
 @param SOCKDATA SOCKDATA指针
 */
void free_sockData(SOCKDATA *);

/**
 创建SOCKCONN
 @param ep_fd  epoll句柄
 @param sock_fd sock句柄
 @param events 关注的事件
 @return SOCKCONN指针
 */
SOCKCONN *malloc_sockconn(int ep_fd, int sock_fd, int events);

/**
 释放创建的SOCKCONN
 @param SOCKCONN SOCKCONN指针
 */
void free_sockconn(SOCKCONN *);

#endif // UTILITY_H_INCLUDED
