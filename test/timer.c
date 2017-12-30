#include <pthread.h>
#include <time.h>
#include "utility.h"
#include "../common/timer.h"

static bool g_stop;

/* Returns an integer in the range [0, n).
 *
 * Uses rand(), and so is affected-by/affects the same seed.
 */
int randint(int n=300) 
{
    if ((n - 1) == RAND_MAX) 
    {
        return rand();
    } 
    else 
    {
        // Chop off all of the values that would cause skew...
        long end = RAND_MAX / n; // truncate skew
        assert (end > 0L);
        end *= n;

        // ... and ignore results from rand() that fall above that limit.
        // (Worst case the loop condition should succeed 50% of the time,
        // so we can expect to bail out of this loop pretty quickly.)
        int r;
        while ((r = rand()) >= end);

        return r % n;
    }
}

void timer_task_func(void* priv)
{
    int sock = *priv;
    dbgprint("it's timeout, close the sock:%d\n", sock);
}

static int sock_id = 0;

void worke_thread()
{
    int timer_fd = 0;
    if(!g_stop)
    {
        // 线程开始后就初始化一个timer, 一个线程对应有一个timer
        if((timer_fd=register_a_timer(300, true, timer_task_func, &(++sock_id)) < 0)
        {
            dbgprint("register a timer failed!\n");
        }
        dbgprint("register a timer with sock:%d successfully!\n", sock_id);
    }

    while(!g_stop && timer_fd>0)
    {
        sleep(randint(600));   

        //try to stop timer
        if(stop_a_timer(timer_fd))
        {
            dbgprint("stop timer:%d\n", timer_fd);
        }
        else
        {
            dbgprint("stop timer:%d\n", timer_fd);
        }

    }
}

void handleInterrupt(int sig)
{
    g_stop = true;
}

int main()
{   
    g_stop = false;
    signal(SIGINT, handleInterrupt);

    srand(time(NULL));               // should only be called once

    // init clock env
    init_clock();

    pthread_t th1;
    int rtn = pthread_create(&th1, NULL, (void*)worke_thread, NULL);
    if(rtn != 0)
    {
        dbgprint("%s:%d:create rworke_thread failed, error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    pthread_t th2;
    int rtn = pthread_create(&th2, NULL, (void*)worke_thread, NULL);
    if(rtn != 0)
    {
        dbgprint("%s:%d:create worke_thread failed, error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    pthread_t th3;
    int rtn = pthread_create(&th3, NULL, (void*)worke_thread, NULL);
    if(rtn != 0)
    {
        dbgprint("%s:%d:create worke_thread failed, error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    pthread_t th4;
    int rtn = pthread_create(&th4, NULL, (void*)worke_thread, NULL);
    if(rtn != 0)
    {
        dbgprint("%s:%d:create worke_thread failed, error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    pthread_t th5;
    int rtn = pthread_create(&th5, NULL, (void*)worke_thread, NULL);
    if(rtn != 0)
    {
        dbgprint("%s:%d:create worke_thread failed, error%d(%s)",__FILE__, __LINE__, errno, strerror(errno));
        return -1;
    }

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);
    pthread_join(th4, NULL);
    pthread_join(th5, NULL);

    // destory clock
    destroy_clock();
}