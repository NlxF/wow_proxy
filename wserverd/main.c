#include "../common/utility.h"

#define SERVERNAME "wserver"
#define LOGPREFIX "WSERVERD:"

int main(int argc, char *argv[])
{
    pid_t pid=0;
    //syslog��ʼ�����ã���������Ϣ���͵�syslogd,
    openlog(LOGPREFIX, LOG_CONS|LOG_PID, LOG_LOCAL0);
    //�����ػ����̣��ر�stdin,stdout,stderr
    if(create_daemon(1, 0)!=0)   //if(daemon(1, 1)==-1)
        exit(-2);

    //�����ӽ���,������ӽ����Ƿ�������˳�
    for(;;)
    {
        if((pid=fork())==-1)
        {
            //������
            psyslog(LOG_ERR, strerror(errno), __FILE__, __FUNCTION__, __LINE__);
            exit(-3);
        }
        else if(pid==0)
        {
            //�ӽ���
            init_child_image(SERVERNAME);
        }
        else if(pid > 0)
        {
            //��ǰ����
            int  status  =0;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status) && (WEXITSTATUS(status)==0))
                exit(0);
        }
    }
}
