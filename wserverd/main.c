#include "../common/utility.h"

#define SERVERNAME "wserver"
#define LOGPREFIX "WSERVERD:"

int main(int argc, char *argv[])
{
    pid_t pid=0;
    //syslog初始化设置，将错误信息发送到syslogd,
    openlog(LOGPREFIX, LOG_CONS|LOG_PID, LOG_LOCAL0);
    //创建守护进程，关闭stdin,stdout,stderr
    if(create_daemon(1, 0)!=0)   //if(daemon(1, 1)==-1)
        exit(-2);

    //创建子进程,并监控子进程是否非正常退出
    for(;;)
    {
        if((pid=fork())==-1)
        {
            //出错了
            psyslog(LOG_ERR, strerror(errno), __FILE__, __FUNCTION__, __LINE__);
            exit(-3);
        }
        else if(pid==0)
        {
            //子进程
            init_child_image(SERVERNAME);
        }
        else if(pid > 0)
        {
            //当前进程
            int  status  =0;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status) && (WEXITSTATUS(status)==0))
                exit(0);
        }
    }
}
