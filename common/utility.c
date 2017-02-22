#include "utility.h"


extern int loop;

//-----------------------------------------------DAEMON-------------------------------------------//
int create_daemon(int nochdir, int noclose)
{
    pid_t pid;
    pid = fork();
    if(pid == -1){
        psyslog(LOG_ERR, strerror(errno), __FILE__, __FUNCTION__, __LINE__);
        return(-1);
    }
    if(pid>0)
        exit(0);
    if(setsid() == -1){
        dbgprint("setsid() Failed with error msg:%s\n", strerror(errno));
        return(-2);
    }
    if(nochdir == 0)
        chdir("/");
    if(noclose == 0)
    {
        int i;
        for(i=0;i<3;++i)
        {
            close(i);
            //open("/dev/null", O_RDWR);
            //dup(0);
            //dup(0);
        }
    }
    umask(0);
    return(0);
}


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
    //dbgprint("current absolute path:%s\n", current_absolute_path);
    return 0;
}

int get_home_Path(char szPath[512], int nbyte)
{
	bzero(szPath, nbyte);
	
	char *home_path = getenv("HOME");
	sprintf(szPath, "%s/%s", home_path, ".wowserver");
	mkdir(szPath, 0777);
	if(errno != 0 || errno != 17)
	{
		dbgprint("%s\n", strerror(errno));
		return -1;
	}
	return 0;
}

void init_child_image(char* servername)
{
    int     log_fd;
    char  *const pargv[2]                                       = {NULL};
    char  argv[2][MAX_SIZE]                              = {0} ;
    char  file_path[MAX_SIZE]                           = {0};
    char  current_absolute_path[MAX_SIZE]     = {0};
    //获取当前目录绝对路径
    if (get_absolute_path(current_absolute_path)!=0)   //if(getcwd(current_absolute_path, MAX_SIZE)==NULL)
    {
        psyslog(LOG_ERR, strerror(errno), __FILE__, __FUNCTION__, __LINE__);
        exit(-1);
    }
    //stdout重定向到./wserver.log
    snprintf(file_path, MAX_SIZE, "%s/%s", current_absolute_path, "wserver.log");
    log_fd = open(file_path, O_CREAT|O_RDWR|O_APPEND, 0666);
    dup2(log_fd, STDOUT_FILENO);

    snprintf(file_path, MAX_SIZE, "%s/%s", current_absolute_path, servername);
    strncpy(argv[0], servername, MAX_SIZE);
    //pargv[0] = argv;
    char *const argv2[] = {servername, NULL};
    execvp(file_path,  argv2);
    psyslog(LOG_ERR, strerror(errno), __FILE__, __FUNCTION__, __LINE__);
    exit(-1);
}
//------------------------------------------------------------------------------------------------//

//---------------------------------------------UTILITY----------------------------------------//
void set_process_max_fd(int max)
{
	struct rlimit rt;
	struct rlimit old_rt;

	if(max == 0)
	  rt.rlim_cur = rt.rlim_max = RLIM_INFINITY;
	else
	  rt.rlim_cur = rt.rlim_max = max;

	if(getrlimit(RLIMIT_NOFILE, &old_rt) == 0)
	{
		if(setrlimit(RLIMIT_NOFILE, &rt) != 0)
		{
			syslog(LOG_WARNING, "SET NOFILE INFINITY FAILED: %m, NOW IS %d\n", old_rt.rlim_max);
			rt.rlim_cur = rt.rlim_max = old_rt.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rt);
		}
		else
		  syslog(LOG_INFO, "SET NOFILE INFINITY...\n");
	}
}


int create_listen_sock(int port)
{
	int    listen_fd;
	struct sockaddr_in addr4;
	const int try_num = 100;

	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		syslog(LOG_ERR, "CREATE LISTEN SOCK FAILED, ERROR CODE:%m\n");
		return -1;
	}

	//设置socket为非阻塞模式
	if(set_non_blocking(listen_fd)!=0)
		return -2;

	//bind port
	bzero(&addr4, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_addr.s_addr = INADDR_ANY;
	int i;
	for(i=0; i<try_num; ++i)
	{
		addr4.sin_port = htons(port+i);
		if(bind(listen_fd, (struct sockaddr*)&addr4, sizeof(addr4))==-1)
		{
			if(errno==EADDRINUSE)
				continue;
			else
			{
				syslog(LOG_ERR, "BIND FAILED, ERROR CODE:%m");
				return -3;
			}
		}
		else
		  break;
	}
	syslog(LOG_ERR, "CREATE LISTENING SERVER AT PORT:%d", i+port);
	return listen_fd;
}

int create_server(int port)
{
    int  listen_fd;
    
    if(port <= 0) return -1;
    
    //create listening socket
    if((listen_fd=create_listen_sock(port)) < 0)
    {
        return -1;
    }
    dbgprint("create listen sock...\n");
    
    //begin listening
    if(listen(listen_fd, LISTEN_MAX) == -1)
    {
        dbgprint("%s:%d:listen sock error:%s\n",__FILE__, __LINE__, strerror(errno));
        return -2;
    }
    
    return listen_fd;
    
}


int set_non_blocking(int sockfd)
{
	if(sockfd <= 0)
	  return -1;

	if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
	{
		syslog(LOG_ERR, "SET SOCKFD NON BLOCKING FAILED,ERROR CODE: %m");
		return -2;
	}

	syslog(LOG_INFO, "SET SOCKFD NON BLOCKING...");
	return 0;
}

int create_epoll(int sock_fd)
{
	int   epfd;

	if((epfd = epoll_create(1024))==-1)  //参数为向前兼容必须大于0
	{
		dbgprint("%s:%d:create epoll error:%s\n",__FILE__, __LINE__, strerror(errno));
		return -1;
	}
    
	return epfd;
}


void add_epoll_fd(SOCKCONN *sockConn)
{
    if(sockConn==NULL)  return;
    
    struct epoll_event event;
    
    memset(&event, 0, sizeof(event));
    //event.data.fd = sockConn->sock_fd;
    event.data.ptr = sockConn;
    event.events = sockConn->events;
    int rtn = epoll_ctl(sockConn->ep_fd, EPOLL_CTL_ADD, sockConn->sock_fd, &event);   //add event
    if (rtn != 0)
    {
        dbgprint("epoll_ctl add failed %d %s\n", errno, strerror(errno));
        return;
    }
    dbgprint("%d accept client :%d\n", ++loop, sockConn->sock_fd);
}

void modify_epoll_fd(SOCKCONN *sockConn)
{
    
    if(sockConn==NULL)  return;
    
    struct epoll_event event;
    
    memset(&event, 0, sizeof(event));
    event.data.ptr = sockConn;
    event.events = sockConn->events;

    int rtn = epoll_ctl(sockConn->ep_fd, EPOLL_CTL_MOD, sockConn->sock_fd, &event);   //modify event
    if (rtn != 0)
    {
        dbgprint("epoll_ctl mod failed %d %s\n", errno, strerror(errno));
        return;
    }
}

bool redirect_namedpip(int *write_fd, int *read_fd)
{
    char l2w[MAX_SIZE] ={0};
    char w2l[MAX_SIZE] = {0};
    char current_abs_path[MAX_SIZE] ={0};
    
    //get_absolute_path(current_abs_path);
    get_home_Path(current_abs_path, sizeof(current_abs_path));
    snprintf(l2w, MAX_SIZE, "%s/%s", current_abs_path,LSERVER2WSERVER);
    snprintf(w2l, MAX_SIZE, "%s/%s", current_abs_path,WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
    {
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
        return false;
    }
    *write_fd = open(l2w, O_RDWR);
    *read_fd = open(w2l, O_RDWR);
    if(*write_fd==-1 || *read_fd==-1)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
        return false;
    }
    
    dbgprint("open pipe....\n");
    
    //set process max fd, 0 for RLIM_INFINITY,
    set_process_max_fd(0);
    
    return true;
}

SOCKCONN *malloc_sockconn(int ep_fd, int sock_fd, int events)
{
    SOCKCONN *sockConn = (SOCKCONN*)malloc(sizeof(SOCKCONN));
    if (sockConn==NULL)
    {
        return NULL;
    }
    memset(sockConn, 0, sizeof(SOCKCONN));
    sockConn->ep_fd = ep_fd;
    sockConn->events = events;
    sockConn->sock_fd = sock_fd;
    
    return sockConn;
}

void free_sockconn(SOCKCONN *sockConn)
{
    if(sockConn != NULL)
    {
        if(sockConn->sock_fd > 0)
        {
            close(sockConn->sock_fd);
        }
#ifdef SOCKSSL
        if(sockConn->ssl)
        {
            SSL_shutdown(sockConn->ssl);
            SSL_free(sockConn->ssl);
        }
#endif
        free(sockConn);
    }
}

SOCKDATA *malloc_sockData(SOCKCONN *sockConn, int read_fd, int write_fd)
{
    SOCKDATA *pdata = (SOCKDATA*)malloc(sizeof(SOCKDATA));
    if(pdata == NULL)
    {
        return NULL;
    }
    bzero(pdata, sizeof(SOCKDATA));
    pdata->sockConn = sockConn;
    pdata->write_fd = write_fd;
    pdata->read_fd = read_fd;
    
    return pdata;
}

void free_sockData(SOCKDATA *sockData)
{
    if (sockData != NULL)
    {
        free(sockData);
    }
}

