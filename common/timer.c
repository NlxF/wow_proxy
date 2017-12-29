#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "timer.h"
#include "utility.h"
#include "uthash/uthash.h"

typedef struct
{
    timer_task_func_t func;
    int sock;
}TIMER_DATA;

typedef struct
 {
    int timer_fd;                    /* key */
    TIMER_DATA* timer_data;
    UT_hash_handle hh;               /* makes this structure hashable */
}HashTable;

static int        g_epoll_fd     = -1;           // epoll 描述符
static bool       g_running      = false;        // 定时器线程是否在工作
static pthread_t  g_epoll_thread = 0;            // 定时器线程
static HashTable* g_timer_table  = NULL;         // 保存所有的timer:TIMER_DATA
static pthread_rwlock_t g_rw_lock;               // 用于uthash的多线程写

static void rest_status()
{
    g_epoll_fd     = -1;
    g_running      = false ;
}

static HashTable* search_hashtable_by(int timer_fd)
{
    if (pthread_rwlock_rdlock(&g_rw_lock) != 0)
    {
        dbgprint("%s:%d:pthread_rwlock_rdlock() error:%d(%s)\n",__FILE__, __LINE__, errno, strerror(errno));
        return NULL;
    }
    HashTable *tmp = NULL;
    HASH_FIND_INT(g_timer_table, &timer_fd, tmp);                 // 查询是否已经添加过
    pthread_rwlock_unlock(&g_rw_lock);

    return tmp;
}

static bool add_a_timer(int timer_fd, TIMER_DATA* timer_data)
{
    if (pthread_rwlock_wrlock(&g_rw_lock) != 0)
    {
        dbgprint("%s:%d:pthread_rwlock_wrlock() error:%d(%s)\n",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }
    HashTable *tmp = NULL;
    HASH_FIND_INT(g_timer_table, &timer_fd, tmp);                 // 查询是否已经添加过
    if (tmp == NULL) 
    {
        tmp = (HashTable*)malloc(sizeof(HashTable));
        tmp->timer_data = timer_data;
        HASH_ADD_INT(g_timer_table, timer_fd, tmp);              // 添加到table
    }
    pthread_rwlock_unlock(&g_rw_lock);

    if(tmp->timer_data)
        free(tmp->timer_data);

    tmp->timer_data = timer_data;
    return true;
}

static bool free_a_timer(int timer_fd)
{
    HashTable* column;

    if (pthread_rwlock_wrlock(&g_rw_lock) != 0)
    {
        dbgprint("%s:%d:pthread_rwlock_wrlock() error:%d(%s)\n",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }
    HASH_FIND_INT(g_timer_table, &timer_fd, column); 
    if (spdata != NULL) 
    {
        HASH_DEL(g_timer_table, column);
        free(column->timer_data);    
        free(column);
        close(column->timer_fd);
    }
    pthread_rwlock_unlock(&g_rw_lock);
    return true;
}

static bool free_timer_table()
{
    HashTable* current_data, tmp;

    if (pthread_rwlock_wrlock(&g_rw_lock) != 0) 
    {
        dbgprint("%s:%d:pthread_rwlock_wrlock() error:%d(%s)\n",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }
    HASH_ITER(hh, g_timer_table, current_data, tmp) 
    {
        HASH_DEL(g_timer_table, current_data);      /* delete it (users advances to next) */
        free(current_data->timer_data);
        free(current_data);                         
        close(current_data->timer_fd);
    }
    pthread_rwlock_unlock(&g_rw_lock);

    return true;
}

static void epoll_loop_run()
{
    pthread_detach(pthread_self());              //设置为detached, 让线程结束时可以释放所有资源
    while(g_running)
    {
        int    nfds;                             //ready I/O fd
        struct epoll_event event;
        struct epoll_event events[LISTEN_MAX];   //all events
        
        nfds = epoll_wait(g_epoll_fd, events, LISTEN_MAX, -1);
        if(nfds == -1)
        {
            dbgprint("%s:%d:epoll_wait() error:%d(%s)\n",__FILE__, __LINE__, errno, strerror(errno));
            if(errno == EINTR)
            {
                continue;                        // 被中断打断,继续wait
            }
            else
            {
                close(g_epoll_fd);
                free_timer_table();             //释放所有的timer及与之关联的data
                rest_status()                   //epoll_wait发生未知错误导致线程退出,重置状态
			    break;
            }
		}
        //handle all events
        int timer_num;
        for(timer_num=0; timer_num<nfds; ++timer_num)
        {   
            int timer_fd = events[timer_num].data.fd;
            int _event = events[timer_num].events;
            if(_event & EPOLLIN)                      //data in include
            {
                // 处理相应的操作
                HashTable* cloumn = search_hashtable_by(timer_fd);
                if(cloumn && cloumn->func)
                {
                    cloumn->func(cloumn->sock);
                }
                else
                {
                    dbgprint("%s:%d:search hashtable for key:%d is NULL", __FILE__, __LINE__, timer_fd);
                }
            }
            else
            {
                dbgprint("%s:%d:timer:occur a unknown event", __FILE__, __LINE__);
            }
        }
    }   
}

static bool init_epoll_thread()
{
    int rtn = pthread_create(&g_epoll_thread, NULL, (void*)epoll_loop_run, NULL);
    if(rtn != 0)
    {
        dbgprint("%s:%d:create epoll thread failed, error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }
}

static int create_timerfd(struct itimerspec *its, time_t interval, bool repeat)
{
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if(timer_fd < 0)
    {
		dbgprint("%s:%d:timerfd_create() error",__FILE__, __LINE__);
		return -2;
	}
	struct timespec nw;
	if(clock_gettime(CLOCK_MONOTONIC, &nw) != 0)
    {
		dbgprint("%s:%d:clock_gettime() error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
		close(timer_fd);
		return -3;
	}
    
	its->it_value.tv_sec = nw.tv_sec + interval;
	its->it_value.tv_nsec = 0;
    its->it_interval.tv_sec = repeat ? interval : 0;
	its->it_interval.tv_nsec = 0;

	return timer_fd;
}

////////////////////////////////////////////////////////////////////
bool init_clock()
{
    if(g_epoll_fd <= 0 && !g_running)
    {
        if (pthread_rwlock_init(&g_rw_lock, NULL) != 0)
        {
            dbgprint("%s:%d:can't create rwlock, error%d(%s)",,__FILE__, __LINE__, errno, strerror(errno));
            return false;
        }

        //create epoll
        g_epoll_fd = create_epoll(listen_fd);
        if (g_epoll_fd < 0)
        {
            return false;
        }

        //init epoll thread
        g_running = true;
        if(!init_epoll_thread())
        {
            g_running = false;
            return false;
        }
    }

    return true;
}

void destroy_clock()
{
    close(g_epoll_fd);
    free_timer_table();                  //释放所有的timer及与之关联的data
    rest_status()                        
    pthread_rwlock_destroy(g_rw_lock);   //销毁读写锁

}

int register_a_timer(time_t delay, bool repeat, timer_task_func_t func, int priv)
{
    if(delay<=0 || g_epoll_fd<=0 || !g_running)
        return -1;
    
	struct itimerspec its;
	int timer_fd = create_timerfd(&its, delay, repeat);
    if(timer_fd < 0)
        return timer_fd;
    
    if(timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &its, NULL) != 0)
    {
        dbgprint("%s:%d:timerfd_settime() error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        close(timer_fd);
        return -1;
    }

    // store timer_fd to hashtable
    TIMER_DATA* column = (TIMER_DATA*)malloc(sizeof(TIMER_DATA));
    column->func = func;
    colun->sock  = priv;
    if(add_a_timer(timer_fd, column))
    {
        // add timerfd to epoll
        struct epoll_event ev;
        ev.events = EPOLLIN;      // | EPOLLET  monitor event  
        ev.data.fd = timer_fd;                  
        if(epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) != 0)
        {
            dbgprint("%s:%d:epoll_ctl(ADD) error:%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
            return -1;
        }
    }
    else
    {
        free(column);
        return -1;
    }
    
    return timer_fd;
}

bool cancel_a_timer(int timer_fd)
{
    if(g_epoll_fd<=0 || !g_running || timer_fd<0)
        return false;

    // remove from epoll events
    if(epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, timer_fd, NULL) != 0)
    {
        dbgprint("%s:%d:epoll_ctl(DEL) error:%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }

    // remove from hashtable
    return free_a_timer(timer_fd);
}

bool stop_a_timer(int timer_fd)
{
    if(g_epoll_fd<=0 || !g_running || timer_fd<0)
        return false;

    //stop timer
    struct itimerspec its;
    its->it_value.tv_sec = 0;
	its->it_value.tv_nsec = 0;
    its->it_interval.tv_sec = 0;
	its->it_interval.tv_nsec = 0;
    if(timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &its, NULL) != 0)
    {
        dbgprint("%s:%d:timerfd_settime() error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }

    return true;
}

bool resume_a_timer(int timer_fd, time_t new_delay, bool repeat)
{
    if(g_epoll_fd<=0 || !g_running || timer_fd<0 || new_delay<=0)
        return false;

    // new 
    struct timespec nw;
	clock_gettime(CLOCK_MONOTONIC, &nw)

    struct itimerspec its;
    its->it_value.tv_sec = nw.tv_sec + new_delay;
	its->it_value.tv_nsec = 0;
    its->it_interval.tv_sec = repeat ? new_delay : 0;
	its->it_interval.tv_nsec = 0;

    if(timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &its, NULL) != 0)
    {
        dbgprint("%s:%d:timerfd_settime() error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return false;
    }

    return true;
}