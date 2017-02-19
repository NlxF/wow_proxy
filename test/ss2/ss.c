#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

///redirection to pipe
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

void RedirectToPip(int *in_fd, int *out_fd)
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
        printf("1 %s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
		return -2;
	}

	int write_fd = open(w2l, O_RDWR);
    int read_fd = open(l2w, O_RDWR);
    if(write_fd==-1||read_fd==-1)
	{
        printf("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
		return -3;
	}

	dup2(write_fd, STDOUT_FILENO);
	dup2(read_fd, STDIN_FILENO);

	//printf("before stdin=%d\n", stdin);
	//FILE *fin = freopen(l2w, "w+", stdin);
	//*fin = fopen(l2w, "w+");
	//if(*fin == NULL)
	{
		//printf("ERROR:%d = %s\n", errno, strerror(errno));
	}
	//printf("after stdin=%d\n", stdin);
	//*fout = freopen(w2l, "w+", stdout);
	//if(*fout == NULL)
	{
		//printf("ERROR:%d = %s\n", errno, strerror(errno));
	}
	
	//*in_fd = open(l2w, O_RDWR);
	//close(0);
    //dup(myFifo);
    //printf("read pip ok...\n");
	//fflush(stdout);
	
	//*out_fd = open(w2l, O_RDWR);
	//close(1);
    //dup(myFifo);
    //printf("write pip ok...\n");
	//fflush(stdout);

	return;
}

int main()
{
	//FILE *fin;
	int in_fd, out_fd;
	RedirectToPip(&in_fd, &out_fd);
    
	while(1)
	{
		char *command_str;
		char commandbuf[256] = {0};

		//printf("read to get command from pip...\n");
		//command_str = fgets(commandbuf, sizeof(commandbuf), stdin);
		read(STDIN_FILENO, commandbuf, 256);
		//printf("recv command=%s\n", commandbuf);
		
		char szOut[256] = {0};
		sprintf(szOut, "recv command=%s and send result=%s", commandbuf, "hello world~!");
		write(STDOUT_FILENO, szOut, strlen(szOut));
	}
}
