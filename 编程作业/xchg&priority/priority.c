#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


//pthread_mutex_t mutex;//set lock
pthread_mutex_t  mutex;
pthread_mutexattr_t mattr;

void * worker_func_low(void *arg)
{
    int j = 100000000;
    pthread_mutex_lock(&mutex);
    printf("thread with low priority enters critical section\n");
    while(j > 0)
    --j;
    printf("Thread with low priority finished\n");
    pthread_mutex_unlock(&mutex);

    return NULL;
};
void * worker_func_high(void *arg)
{
   sleep(0.3);
    int j = 100000000;
    pthread_mutex_lock(&mutex);
     printf("thread with high priority enters the critical section\n");
    while(j > 0)
    --j;
   printf("Thread with high priority finished\n");
   pthread_mutex_unlock(&mutex);

    return NULL;
};


int main(void)
{
    pthread_t thread_low, thread_high;
    struct sched_param param_low;//priority
    struct sched_param param_high;
   
    pthread_mutex_init(&mutex,&mattr);

    pthread_attr_t attr_low;
    pthread_attr_t attr_high;
    pthread_attr_init(&attr_low);
    pthread_attr_init(&attr_high);
    //refuse to inherit the priority of parent thread, root
   pthread_attr_setinheritsched(&attr_low,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attr_high,PTHREAD_EXPLICIT_SCHED);
    //set scheduling policy
    pthread_attr_setschedpolicy(&attr_low,SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr_high,SCHED_FIFO);
    //set priority
    param_low.sched_priority = 1;
    param_high.sched_priority = 88;
    pthread_attr_setschedparam(&attr_low,&param_low);
    pthread_attr_setschedparam(&attr_high, &param_high);
    //shared lock
   // pthread_mutexattr_setpshared(&attr_low,PTHREAD_PROCESS_PRIVATE);
   // pthread_mutexattr_setpshared(&attr_high,PTHREAD_PROCESS_PRIVATE);
    int x = pthread_create(&thread_low, &attr_low, worker_func_low, NULL);
    int y = pthread_create(&thread_high, &attr_high, worker_func_high, NULL);

    pthread_join(thread_low, NULL);
    pthread_join(thread_high, NULL);

    printf("both threads finished\n%d\n%d\n", x, y);

    return 0;
}
