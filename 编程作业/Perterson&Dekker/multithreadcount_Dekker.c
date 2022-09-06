#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM 10

int count = 0;
int inside[3] = {0};
int turn = 1;

void * worker_func_thread1(void *arg)
{
    for (int i = 0; i < NUM; i++) { 
            inside[1] = 1;// thread1 wants to use critical section
            while(inside[2] )
            {
                if (turn == 2)
                {
                    inside[1] =0;//give up critcal section
                    while (turn == 2)
                    ;//warit the thread2 give up critical section
                    inside[1] =1;
                }
            }
            count++;
            printf("thread1 occupy critcal section\n");
            turn = 2;
            inside[1] = 0;
    }

    return NULL;
}
void * worker_func_thread2(void *arg)
{
    for (int i = 0; i < NUM; i++) { 
            inside[2] = 1;// thread2 wants to use critical section
            while(inside[1] )
            {
                if (turn == 1)
                {
                    inside[2] =0;//give up critcal section
                    while (turn == 1)
                    ;//warit the thread2 give up critical section
                    inside[2] =1;
                }
            }
            count++;
            printf("thread2 occupy critcal section\n");
            turn = 1;
            inside[2] = 0;
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
