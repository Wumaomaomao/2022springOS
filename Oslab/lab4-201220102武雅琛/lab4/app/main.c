#include "lib.h"
#include "types.h"
#define N 5
#define SamphoreTest
//#define philosopher
//#define ProducerConsumer
//#define ReadWrite

void philosophereat(int pid, sem_t* knife);

void Producer(int idx, sem_t fullplace, sem_t emptyplace);
void Consumer(sem_t fullplace, sem_t emptyplace);

void ReadProcess(int pid, sem_t Writelock, sem_t mutex);
void WriteProcess(int pid, sem_t Writelock);

int uEntry(void) {

#ifdef SamphoreTest
	// 测试scanf	
	int dec = 0;
	int hex = 0;
	char str[6];
	char cha = 0;
	int ret = 0;
	while(1){
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
	}
	
	// 测试信号量
	int i = 4;
	sem_t sem;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 0);
	printf("init semaphore:%d\n", sem);
	if (ret == -1) {
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0) {
		printf("Child Process pid:%d\n",getpid());
		while( i != 0) {
			i --;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1) {
		printf("Father Process pid:%d\n",getpid());
		while( i != 0) {
			i --;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}

#elif defined philosopher
	// For lab4.3
	// TODO: You need to design and test the philosopher problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.
	
	//哲学家
	printf("Philosopher problem test start!\n");
	//int pid = getpid();
	sem_t knife[5];

	for (int i = 0; i < 5; ++ i)
	{
		int ret = sem_init(&(knife[i]), 1);
		if (ret != -1)
		{
			printf("Initialize knife: knife %d semaphore %d\n", i, knife[i]);
		}
		else
		{
			printf("Semaphore Initializing Failed.\n");
			exit();
		}
	}

	int pid;
	pid = getpid();
	printf("Father Process pid: %d\n", pid);
	for (int i = 0; i < 4; ++ i)
	{
		int forkret;
		forkret = fork();
		if (forkret == 0)
		{
			pid = getpid();
			printf("Child Process pid: %d\n", pid);
			break;
		}
	}
	//sleep(1000);
	philosophereat(getpid(), knife);
	//生产者消费者问题

	//读者写者问题
	





#elif defined ProducerConsumer

	sem_t emptyplace, fullplace;
	int ret;

	ret = sem_init(&emptyplace, N);
	if (ret != -1)
	{
		printf("Initialize emptyplace semaphore: %d, value: %d\n", emptyplace, N);
	}
	sem_init(&fullplace, 0);
	if (ret != -1)
	{
		printf("Initialize fullplace semaphore: %d, value: %d\n", fullplace, 0);
	}
	
	int pid = getpid();
	printf("Father Process pid: %d\n", pid);
	for (int i = 0; i < 4; ++ i)
	{
		ret = fork();
		if (ret == 0)
		{
			pid = getpid();
			printf("Child Process pid: %d\n", pid);
			break;
		}
	}
	if (ret == 0)
	{
		Producer(pid - 1, fullplace, emptyplace);
	}
	else
	{
		Consumer(fullplace, emptyplace);
	}

	sleep(1234);

#elif defined ReadWrite
	/*int test;
	test = ReadCount();
	printf("%d\n", test);
	test = ModifyCount(1);
	printf("%d\n", test);
	test = ModifyCount(-1);
	printf("%d\n", test);
	*/
	int ret;
	sem_t Writelock, mutex;
	ret = sem_init(&Writelock, 1);
	if (ret != -1)
	{
		printf("Initialize Writelock semaphore: %d, value: %d\n", Writelock, 1);
	}
	sem_init(&mutex, 1);
	if (ret != -1)
	{
		printf("Initialize mutex semaphore: %d, value: %d\n", mutex, 1);
	}
	int pid = getpid();
	printf("Father Process pid: %d\n", pid);
	for (int i = 0; i < 3; ++ i)
	{
		ret = fork();
		if (ret == 0)
		{
			pid = getpid();
			printf("Child Process pid: %d\n", pid);
			break;
		}
	}
	if (ret != 0)
	{
		for (int i = 0; i < 2; ++ i)
		{
			ret = fork();
			if (ret == 0)
			{
				pid = getpid();
				printf("Child Process pid: %d\n", pid);
				break;
			}
		}
	}

	pid = getpid();

	if (1 <= pid && pid <= 3)
	{
		ReadProcess(pid, Writelock, mutex);
	}
	
	if (4 <= pid && pid <= 6)
	{
		WriteProcess(pid - 3, Writelock);
	}
#endif
	exit(0);
	return 0;
}

void philosophereat(int pid, sem_t* knife)
{
	int leftknife = ((pid - 1) + N) % N;
	int rightknife = ((pid - 1) + 1 + N) % N;
	printf("philosopher: %d leftknife: %d rightknife: %d\n", pid, leftknife, rightknife);
	
	while(1)
	{
		printf("Philosopher %d: think\n", pid);
		sleep(128);
	
	
		if (pid % 2 == 0)
		{
			sem_wait(&(knife[leftknife]));
			printf("Philosopher %d: pick up knife %d\n", pid, leftknife);
			sleep(128);
			sem_wait(&(knife[rightknife]));
			printf("Philosopher %d: pick up knife %d\n", pid, rightknife);
			sleep(128);
			printf("Philosopher %d: eat with %d and %d\n", pid, leftknife, rightknife);
			sleep(128);
			sem_post(&(knife[leftknife]));
			printf("Philosopher %d: pick down knife %d\n", pid, leftknife);
			sleep(128);
			sem_post(&(knife[rightknife]));
			printf("Philosopher %d: pick down knife %d\n", pid, rightknife);
			sleep(128);
		}
		else
		{
			sem_wait(&(knife[rightknife]));
			printf("Philosopher %d: pick up knife %d\n", pid, rightknife);
			sleep(128);
			sem_wait(&(knife[leftknife]));
			printf("Philosopher %d: pick up knife %d\n", pid, leftknife);
			sleep(128);
			printf("Philosopher %d: eat with %d and %d\n", pid, leftknife, rightknife);
			sleep(128);
			sem_post(&(knife[rightknife]));
			printf("Philosopher %d: pick down knife %d\n", pid, rightknife);
			sleep(128);
			sem_post(&(knife[leftknife]));
			printf("Philosopher %d: pick down knife %d\n", pid, leftknife);
			sleep(128);
		}
	}
}

void Producer(int idx, sem_t fullplace, sem_t emptyplace)
{
	while(1)
	{
	printf("Producer %d: producing...\n", idx);
	sleep(1024);

	sem_wait(&emptyplace);
	sleep(128);
	printf("Producer %d: produce\n", idx);
	sleep(128);
	sem_post(&fullplace);
	sleep(128);
	}

}

void Consumer(sem_t fullplace, sem_t emptyplace)
{
	while(1)
	{
	printf("Consumer waiting...\n");
	sleep(1);

	sem_wait(&fullplace);
	//sleep(128);
	printf("Consumer consume...\n");
	//sleep(128);
	sem_post(&emptyplace);
	//sleep(128);
	}
}

void ReadProcess(int pid, sem_t Writelock, sem_t mutex)
{
	int count;
	while(1)
	{
		printf("Read process: %d, waiting...\n", pid);
		sleep(1024);

		sem_wait(&mutex);
		sleep(128);
		count = ModifyCount(1);//++count;
		if (count == 1)
		{
			sem_wait(&Writelock);
			printf("Readprocess obtain ReadWritelock\n");
			sleep(128);
		}
		sem_post(&mutex);

		sleep(128);
		printf("Reader %d: read, total %d reader\n", pid, count);
		sleep(128);

		sem_wait(&mutex);
		sleep(128);
		count = ModifyCount(-1);
		if (count == 0)
		{
			printf("release ReadWritelock\n");
			sem_post(&Writelock);
		}
		sem_post(&mutex);
	}
}

void WriteProcess(int pid, sem_t Writelock)
{
	while(1)
	{
		printf("Write process: %d, waiting...\n", pid);
		sleep(128);

		sem_wait(&Writelock);
		sleep(128);
		printf("Write process obtain ReadWritelock\n");
		printf("Writer %d: write\n", pid);
		sleep(128);
		sem_post(&Writelock);
	}
}