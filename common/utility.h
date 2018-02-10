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
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <netinet/tcp.h>
#include <linux/tcp.h>
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
 创建两条命名管道，一条用于写，一条用于读
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
 检测对端是否已关闭socket
 @param: 要检测的sock
 @return：关闭返回true
*/
bool is_sock_closed_by_peer(int sock);

/**
 重新设置线程本地存储的soap sock
 @return: 成功返回新的soap sock,失败返回<0
*/
int reset_thread_local_soap_socket();

/**
 尝试向描述符写入n个字节
 @param: 要写入的描述符指针，指针是用来回传更新的sock
 @param: 内容缓存
 @param: 写入的字节
 @return: 成功返回写入的字节，失败返回-1
 */
ssize_t writen_fd(int *fd, const void * vptr, size_t n);

/**
 尝试从描述符读取x个字节
 @param: 读取的描述符
 @param: 缓冲区
 @param: 缓冲区大小
 @return: 成功返回读取的字节，失败返回-1
 */
ssize_t readn_fd(int fd_read, char *szData, size_t nData);

/**
 * 向client返回soap server的错误信息
 @param sockData 发送信息
 @param err 错误类型
*/
void write_fd_error_message(SOCKDATA *sockData, int err);

/**
说明：创建一个线程相关的soap socket
返回值: 成功返回socket fd，失败返回值<0
 */
int make_soap_socket();

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
 @param container  读、写管道fd或者socket table
 @return SOCKDATA指针
 */
SOCKDATA *malloc_sockData(SOCKCONN *sockConn, int container[2]/*int read_fd, int write_fd*/);

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
