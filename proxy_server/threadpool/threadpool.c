#include "config_.h"
#include "global.h"
#include "threadpool.h"
#include "../../common/config_.h"


static int threadpool_keepalive = 1;


pthread_mutex_t mutex; // = PTHREAD_MUTEX_INITIALIZER; /*USED TO SERIALIZE QUEUE ACCESS*/


int get_cpu_num()
{
    int num = sysconf(_SC_NPROCESSORS_ONLN);
    return num<1?1:num;
}

//init
threadpool_job_node * threadpool_job_queue_remove_first(threadpool *pthpool)
{
	if(pthpool == NULL)
	  return NULL;

	threadpool_job_node *pfirst_job;
	pfirst_job = pthpool->job_queue->head;
	switch(pthpool->job_queue->job_num)
	{
		case 0:
			return NULL;
		case 1:
			pthpool->job_queue->head = NULL;
			pthpool->job_queue->tail = NULL;
			break;
		default:
			pfirst_job->next->prev = NULL;
			pthpool->job_queue->head = pfirst_job->next;
	}
	(pthpool->job_queue->job_num)--;

	return pfirst_job;
}

void threadpool_thread_run(threadpool *pthpool)
{
	while(threadpool_keepalive == 1)
	{
		if(sem_wait(pthpool->job_queue->queue_sem))
		{
			perror("threadpool_thread_do(): waiting for semaphore\n");
			exit(-1);
		}
		
		if(threadpool_keepalive)
		{

			FUNC function;
			void *argv = NULL;
			threadpool_job_node *pjob_node;

			pthread_mutex_lock(&mutex);
			pjob_node = threadpool_job_queue_remove_first(pthpool);
			pthread_mutex_unlock(&mutex);
			
			//sem_post(pthpool->job_queue->queue_sem);

			if(pjob_node != NULL)
			{
				function = pjob_node->function;
				argv = pjob_node->arg;
				//dbgprint("client %d has data to do...\n", ((SOCKDATA*)argv)->sock_fd);
				function(argv);
				free(pjob_node);
				
				//dbgprint("client %d finish!\n", ((SOCKDATA*)argv)->sock_fd);
			}
		}
		else
		  return;
	}
	return;
}


int threadpool_job_queue_init(threadpool *pthpool)
{
	pthpool->job_queue = (threadpool_job_queue*)malloc(sizeof(threadpool_job_queue));
	if(pthpool->job_queue==NULL)
	    return -1;
	pthpool->job_queue->tail = NULL;
	pthpool->job_queue->head = NULL;
	pthpool->job_queue->job_num = 0;
	return 0;
}

threadpool *threadpool_init(int thread_num)
{
	threadpool *pthpool = NULL;
	if(thread_num <= 0)
	    thread_num = 1;
	
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr); 
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&mutex, &attr);

	pthpool = (threadpool*)malloc(sizeof(threadpool));
	if(pthpool==NULL)
	{
		dbgprint("malloc threadpool struct error\n");
		return NULL;
	}

	pthpool->threads_num = thread_num;
	pthpool->pthreads = (pthread_t*)malloc(thread_num*sizeof(pthread_t));
	if(pthpool->pthreads == NULL)
	{
		dbgprint("malloc threadpool->pthreads error\n");
		return NULL;
	}

	if(threadpool_job_queue_init(pthpool))
	    return NULL;


	pthpool->job_queue->queue_sem = (sem_t*)malloc(sizeof(sem_t));
	if(sem_init(pthpool->job_queue->queue_sem, 0, 0)==-1)   //when init threadpool block all theads;
	{
	    dbgprint("sem_init() failed\n");
	    return NULL;
	}
	int t;
	for(t=0; t<thread_num; ++t)
	    pthread_create(&(pthpool->pthreads[t]), NULL, (void*)threadpool_thread_run, (void*)pthpool);
	return pthpool;
}

//add
void threadpool_job_queue_add(threadpool *pthpool, threadpool_job_node *new_node)
{
	new_node->next = NULL;
	new_node->prev = NULL;

	switch(pthpool->job_queue->job_num)
	{
		case 0:
			pthpool->job_queue->head = new_node;
			pthpool->job_queue->tail = new_node;
			break;
		default:
			new_node->prev = pthpool->job_queue->tail;
			pthpool->job_queue->tail->next = new_node;
			pthpool->job_queue->tail = new_node;
			break;
	}
	pthpool->job_queue->job_num++;
	//only node num le than thread num can post sem
	//if(pthpool->job_queue->job_num <= pthpool->threads_num)
	{
		//int reval;
		//sem_getvalue(pthpool->job_queue->queue_sem, &reval);
		sem_post(pthpool->job_queue->queue_sem);
	}

	return;
}

int threadpool_add_work(threadpool *pthpool, FUNC function, void *argv)
{
	threadpool_job_node *job_node;
	job_node = (threadpool_job_node*)malloc(sizeof(threadpool_job_node));

	if(job_node == NULL)
	{
		fprintf(stderr, "threadpool_add_work():Could not allocate memory for new job\n");
		exit(-1);
	}
	job_node->function = function;
	job_node->arg = argv;
	pthread_mutex_lock(&mutex);
	threadpool_job_queue_add(pthpool, job_node);
	pthread_mutex_unlock(&mutex);
	return 0;
}

//destroy
void threadpool_job_queue_empty(threadpool *pthpool)
{
	threadpool_job_node *tail;
	threadpool_job_node *head;
	threadpool_job_node *current;

	current = head = pthpool->job_queue->head;
	tail = pthpool->job_queue->tail;

	while(pthpool->job_queue->job_num)
	{

		head = current->next;
		free(current);
		current = head;
		(pthpool->job_queue->job_num)--;
	}

	head = NULL;
	tail = NULL;
}

void threadpool_destroy(threadpool *pthpool)
{
	int i;
	threadpool_keepalive = 0;

	for(i=0; i<(pthpool->threads_num); ++i)    //for end all threads;
	{
		if(sem_post(pthpool->job_queue->queue_sem))
		  fprintf(stderr, "threadpool_destroy(): could not bypass sem_wait()\n");
	}

	for(i=0; i<(pthpool->threads_num); ++i)   //wait for all therads terminate
	    pthread_join(pthpool->pthreads[i], NULL);

	if(sem_destroy(pthpool->job_queue->queue_sem) != 0)
	    fprintf(stderr, "threadpool_destroy(): Could not destroy semaphore\n");


	threadpool_job_queue_empty(pthpool);

	free(pthpool->pthreads);
	free(pthpool->job_queue->queue_sem);
	free(pthpool->job_queue);
	free(pthpool);
}
