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

typedef struct 
{
	int  len;
	char msg[1024];
}SOCKDATA, *PSOCKDATA;

int get_absolute_path(char *current_absolute_path)
{
    int cnt = readlink("/proc/self/exe", current_absolute_path, MAX_SIZE);
    if (cnt < 0 || cnt >= MAX_SIZE)
        return -1;
    //获取当前目录绝对路径，即去掉程序名
    int i;
    for (i = cnt; i >=0; --i)
    {
        if (current_absolute_path[i] == '/')
        {
            current_absolute_path[i] = '\0';
            break;
        }
    }
    return 0;
}

void do_work(PSOCKDATA p)
{
	time_t t;

	time(&t);
	char *pdate = asctime(gmtime(&t));
	sleep(1);
	snprintf(p->msg, 1024, "handle all data at %s\n", pdate);
	p->len = strlen(p->msg);
	
}

int main()
{
	//redirection to pipe
	char current_abs_path[MAX_SIZE] ={0};     
	char l2w[MAX_SIZE] ={0};
	char w2l[MAX_SIZE] = {0};

	get_absolute_path(current_abs_path);
	snprintf(l2w, MAX_SIZE, "%s/%s", current_abs_path,LSERVER2WSERVER);
	snprintf(w2l, MAX_SIZE, "%s/%s", current_abs_path,WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
	{
        printf("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
		return -1;
	}
	
	freopen(l2w, "w+", stdin);
	freopen(w2l, "w+", stdout);
    /*int write_fd = open(w2l, O_RDWR);
    int read_fd = open(l2w, O_RDWR);
    if(write_fd==-1||read_fd==-1)
	{
        printf("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
		return -1;
	}*/

	//dup2(write_fd, STDOUT_FILENO);
	//dup2(read_fd, STDIN_FILENO);

	//printf("starting reading/writing\n");
	SOCKDATA data;
	bzero(&data, sizeof(data));
	while(1)
	{
		time_t t;
		//int nbytes = read(read_fd, data.msg, 1024);
		char *command_str = readline(NULL);
		data.len = strlen(command_str);
		//data.msg[nbytes] = '\0';
		time(&t);
		//printf("\n-----------------\n");
		//printf("%sRecv: %s\n", asctime(gmtime(&t)), data.msg);

		do_work(&data);

		//printf(data.msg);
		//printf("\n-----------------\n");		
	}
	//printf("end read/write\n");
}
