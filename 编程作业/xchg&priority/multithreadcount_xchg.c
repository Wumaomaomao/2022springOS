#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM 50000

int count = 0;
int lock = 0;

static inline int lock_xchg()
{
   int t = 1;

    asm volatile ("xchg %0, %1"
                : "=r"(t), "=m"(lock)
                : "0"(t)
                :);
            return t;
}
static inline int unlock_xchg()
{
   int t = 0;

    asm volatile ("xchg %0, %1"
                : "=r"(t), "=m"(lock)
                : "0"(t)
                :);
            return t;
}

void * worker_func_thread1(void *arg)
{
    for (int i = 0; i < NUM; i++) { 
        while(lock_xchg() == 1)
        ;
        count++;
        unlock_xchg();
    }

    return NULL;
}
void * worker_func_thread2(void *arg)
{
    for (int i = 0; i < NUM; i++) { 
        while(lock_xchg() == 1)
        ;
        count++;
        unlock_xchg();
    }

    return NULL;
}


int main(void)
{
    pthread_t worker1, worker2;
    void *worker1_status;
    void *worker2_status;

    pthread_create(&worker1, NULL, worker_func_thread1, NULL);
    pthread_create(&worker2, NULL, worker_func_thread2, NULL);

    pthread_join(worker1, &worker1_status);
    pthread_join(worker2, &worker2_status);

    printf("Count: %d\n", count);

    return 0;
}
