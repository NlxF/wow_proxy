#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/select.h>
#include <unistd.h>

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
		printf("%d=%s\n", errno, strerror(errno));
		return -1;
	}
	return 0;
}

int main()
{
	char szpath[512];
	if(getPipPath(szpath, sizeof(szpath)) != 0)
		return;

	char l2w[MAX_SIZE] ={0};
	char w2l[MAX_SIZE] = {0};

	snprintf(l2w, MAX_SIZE, "%s/%s", szpath, LSERVER2WSERVER);
	snprintf(w2l, MAX_SIZE, "%s/%s", szpath, WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
	{
        printf("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
        unlink(l2w);
        unlink(w2l);
		return;
	}

	int read_fd = open(w2l, O_RDWR);
    //int write_fd = open(l2w, O_RDWR);
    if(/*write_fd==-1||*/read_fd==-1)
	{
        printf("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
		return ;
	}

	//dup2(write_fd, STDOUT_FILENO);
	dup2(read_fd, STDIN_FILENO);

	int out_fd = open(l2w, O_RDWR);
	if(out_fd < 0)
	{
		printf("%d=%s\n", errno, strerror(errno));
		return -1;
	}
	int in_fd = open(w2l, O_RDWR);
	if(in_fd < 0)
	{
		printf("%d=%s\n", errno, strerror(errno));
		return -1;
	}
	int retval;
	//int retval = ftruncate(in_fd, 0);
	//if(retval == -1)
	{
		//printf("1 ftruncate error,fd=%d, errno=%d, msg=%s\n", in_fd, errno, strerror(errno));
		//return;
	}
	//retval = lseek(in_fd, 0, SEEK_SET);
	//if(retval == -1)
	{
		//printf("1 lseek error, msg=%s\n", strerror(errno));	
		//return;
	}
	while(1)
	{
		char *command_line = readline("Please input:");
		printf("INPUT:%s\n", command_line);
		char szCmd[256] = {0};
		sprintf(szCmd, "%s\n", command_line);
		
		ssize_t nbyte = write(out_fd, szCmd, strlen(szCmd));
		printf("write %d byte to pip and ready to read from pip\n", nbyte);

		fd_set rfds;
		struct timeval tv;
	
		FD_ZERO(&rfds);
		FD_SET(in_fd, &rfds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

       	retval = select(in_fd+1, &rfds, NULL, NULL, &tv);
		if(retval > 0)
		{
			usleep(100*1000);
			if(FD_ISSET(in_fd, &rfds))
			{
				char szBuf[256] = {0};
				char szLBuf[4*1024] = {0};
				ssize_t nReadByte;
				int idx = 1;
				do
				{
					printf("at idx:%d\n", idx++);
					nReadByte = read(in_fd, szBuf, 256);
					strcat(szLBuf, szBuf);
				}while(nReadByte == 256);
				printf("OUTPUT RESULT: \n%s\n", szLBuf);
			}
		}
		else
		{
			printf("select timeout!\n");
		}
						
	}		
}


