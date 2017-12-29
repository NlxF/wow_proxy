#ifndef _TIMER_H_
#define _TIMER_H_

typedef void   (*timer_task_func_t)(void* priv);

/**
*  "时钟"初始化, 会开启一个新的线程用于epoll各个timer_fd
*/
bool init_clock();

/**
 * 销毁定时器 
*/
bool destroy_clock();

/**
*  注册一个定时器
*/
int register_a_timer(time_t delay, bool repeat, timer_task_func_t func, int priv);

/**
 * 移除一个定时器
*/
bool cancel_a_timer(int timer_fd);

/**
* 暂停一个定时器
*/
bool stop_a_timer(int timer_fd);

/**
 *  恢复一个定时器
*/
bool resume_a_timer(int timer_fd, time_t new_delay);

#endif