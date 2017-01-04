#include "../common/config.h"
#include "../common/utility.h"
#include "threadpool/threadpool.h"
#include "threadpool/aidsock.h"

const char IMAGENAME[] = {"lserver:"};


int main(int argc, char*argv[])
{
    int  idx;
    int  read_fd;      //read fd
    int  write_fd;     //write fd
    int  listen_fd;    //listen fd
    int  client_fd;    //client fd
    int  ep_fd;        //epoll
    int  nfds;         //ready I/O fd
	char current_abs_path[MAX_SIZE] ={0};
	char l2w[MAX_SIZE] ={0};
	char w2l[MAX_SIZE] = {0};

    //open syslog
    openlog(IMAGENAME, LOG_CONS | LOG_PID, 0);

	//get_absolute_path(current_abs_path);
	get_home_Path(current_abs_path, sizeof(current_abs_path));
	snprintf(l2w, MAX_SIZE, "%s/%s", current_abs_path,LSERVER2WSERVER);
	snprintf(w2l, MAX_SIZE, "%s/%s", current_abs_path,WSERVER2LSERVER);
    if((mkfifo(l2w, 0666)==-1&&errno!=EEXIST)||(mkfifo(w2l,0666)==-1&&errno!=EEXIST))
	{
        dbgprint("%s:%d:%s\n", __FILE__, __LINE__,strerror(errno));
		return -1;
	}
    write_fd = open(l2w, O_RDWR);
    read_fd = open(w2l, O_RDWR);
    if(write_fd==-1||read_fd==-1)
	{
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "open pipe error!");
		return -1;
	}

    printf("open pipe....\n");
    //set process max fd, 0 for RLIM_INFINITY,
    set_process_max_fd(0);

    //create threadpool
    threadpool *pthpool;
    pthpool = threadpool_init(get_cpu_num());
    if(pthpool == NULL)
    {
        dbgprint("%s:%d:%s\n",__FILE__, __LINE__, "create threadpool error");
        return -1;
    }
	printf("create threadpool...\n");

    //create listening socket
    if((listen_fd=create_listen_sock(8083)) < 0)
        return -1;
	printf("create listen sock...\n");

    //begin listening
    if(listen(listen_fd, LISTEN_MAX) == -1)
    {
        dbgprint("%s:%d:listen sock error:%s\n",__FILE__, __LINE__, strerror(errno));
        return -2;
    }
	printf("start listening...\n");

    //create epoll fd
    if((ep_fd=create_epoll(listen_fd)) < 0)
    {
        dbgprint("%s:%d:create epoll error:%s\n",__FILE__, __LINE__, strerror(errno));
        return -3;
    }

    struct epoll_event event;    //
    struct epoll_event events[LISTEN_MAX];   //all events
    //loop
	printf("loop...\n");

	int loop = 0;
	int jobs = 0;
    while(1)
    {
        nfds = epoll_wait(ep_fd, events, LISTEN_MAX, -1);
        //handle all events
        for(idx=0; idx<nfds; ++idx)
        {
			if((events[idx].events & EPOLLERR) ||
			   (events[idx].events & EPOLLHUP) ||
			   (!(events[idx].events & EPOLLIN)))
			{
				//an error has occured on this fd
				dbgprint("%s:%d:An error:%d(%s) has occured on this fd:%d, or the socket is not ready for reading%s\n",__FILE__, __LINE__, errno, strerror(errno), events[idx].data.fd);
                close(events[idx].data.fd);
                continue;
			}
            else if(events[idx].data.fd == listen_fd)
            {
                // We have a notification on the listening socket, which means one or more incoming connections.
				while(1)
				{
                    struct sockaddr_in client_addr;
    	            socklen_t addr_len = sizeof(client_addr);

    	            client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
                    if(client_fd == -1)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            //We have processed all incoming connections.
                            break;
                        }
                        else
                        {
                            dbgprint("%s:%d:accept client error%s\n",__FILE__, __LINE__, strerror(errno));
                            break; //return -4;
                        }
                    }
                    //Make the incoming socket non-blocking and add it to the list of fds to monitor
                    set_non_blocking(client_fd);
                    event.data.fd = client_fd;
                    event.events = EPOLLIN|EPOLLET;
                    epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &event);      //add new event
                    printf("%d accept client :%d\n", ++loop, client_fd);
				}
				continue;
            }
            else if(events[idx].events & EPOLLIN)            //data in
            {
                SOCKDATA *pdata = (SOCKDATA*)malloc(sizeof(SOCKDATA));
                bzero(pdata, sizeof(SOCKDATA));
                pdata->sock_fd = events[idx].data.fd;
				pdata->write_fd = write_fd;
				pdata->read_fd = read_fd;
                threadpool_add_work(pthpool, (void*)read_sock, (void*)pdata);

				//printf("job number :%d, with client: %d\n", ++jobs, events[idx].data.fd);
            }
           else if(events[idx].events & EPOLLOUT)         //data out
           {
			   printf("client: %d send data out", events[idx].data.fd);
               SOCKDAT *pdata = (SOCKDATA*)malloc(sizeof(SOCKDATA));
               bzero(pdata, sizeof(SOCKDATA));
               pdata.fd = events[idx].data.fd;
               threadpool_add_work(pthpool, (void*)write_sock, (void*)pdata);
           }
        }
    }
    close(listen_fd);
    closelog();
}
