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


#define MAX_SIZE 1024

#define psyslog(level, format, ...) \
        syslog(level, format, ##__VA_ARGS__)


#define PRINT_BASE(format, ...)        printf(format, ##__VA_ARGS__)
#define PRINT_0( format, ... ) \
{ \
        time_t timep; \
        time( &timep ); \
        struct tm *pTM = gmtime( &timep ); \
        PRINT_BASE( "[%4d-%02d-%02d %02d:%02d:%02d]"format, \
                                  pTM->tm_year+1900, pTM->tm_mon+1, pTM->tm_mday, pTM->tm_hour+8, pTM->tm_min, pTM->tm_sec, \
                                  ##__VA_ARGS__ ) ; \
}
#define dbgprint(format, ...)  PRINT_0("[%s]"format, IMAGENAME, ##__VA_ARGS__)

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
说明：默认在端口8083
参数：端口号
返回值：成功返回监听套接字，失败返回<0
**/
int create_listen_sock(int port);

/**
说明：设置fd为非阻塞模式
参数：文件描述符
返回值：成功返回0,失败返回<0
**/
int set_non_blocking(int fd);

/**
说明：创建epoll,并将监听sock添加到epoll集合
参数：文件描述符
返回值：成功返回epoll文件描述符，失败返回值<0
**/
int create_epoll(int sock_fd);



#endif // UTILITY_H_INCLUDED
