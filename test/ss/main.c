#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define LSERVER2WSERVER "/.lserver2wserver"
#define WSERVER2LSERVER "/.wserver2lserver"
#define MAX_SIZE 1024

int getPipPath(char szPath[512], int nbyte)
{
    bzero(szPath, nbyte);
    
    char *home_path = getenv("HOME");
    sprintf(szPath, "%s/%s", home_path, ".wowserver");
    mkdir(szPath, 0777);
    if(errno != 0 && errno != 17)
    {
        printf("error:%d = %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int RedirectToPip()
{
    //printf("It will redirect to pip for automation");
    char szpath[512];
    if(getPipPath(szpath, sizeof(szpath)) != 0)
        return -1;
    
    char l2w[MAX_SIZE] ={0};
    char w2l[MAX_SIZE] = {0};
    
    snprintf(l2w, MAX_SIZE, "%s/%s", szpath, LSERVER2WSERVER);
    snprintf(w2l, MAX_SIZE, "%s/%s", szpath, WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
    {
        printf("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
        unlink(l2w);
        unlink(w2l);
        return -2;
    }
    
//    int write_fd = open(w2l, O_RDWR);
    int read_fd = open(l2w, O_RDWR);
    if(/*write_fd==-1||*/read_fd==-1)
    {
        printf("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
        return -3;
    }
    
//    dup2(write_fd, STDOUT_FILENO);
    dup2(read_fd, STDIN_FILENO);
    
    return 1;
}

int main()
{
	//redirection to pipe
    if(RedirectToPip()<0)
    {
        printf("redirect to pip failed\n");
        return;
    }
    printf("redirect to pip...\n");
    
	while(1)
	{
		time_t t;
        
		//int nbytes = read(read_fd, data.msg, 1024);
        printf("read fron stdin...\n");
		char *command_str = readline(NULL);
		
		time(&t);
		printf("\n-----------------\n");
		printf("%sRecv: %s\n", asctime(gmtime(&t)), command_str);

		//printf(data.msg);
		//printf("\n-----------------\n");		
	}
	printf("end read/write\n");
}
