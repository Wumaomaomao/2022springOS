#include "lib.h"
#include "types.h"



int uEntry(void) {
	uint16_t selector;
	//uint16_t selector = 16;
	asm volatile("movw %%ss, %0":"=m"(selector)); //XXX necessary or not, iret may reset ds in QEMU
	asm volatile("movw %%ax, %%ds"::"a"(selector));
	
	int data = 0;	
	//printf("start\n");
	int ret = fork();
	//if (ret == 0)
	//printf("%d\n", 0);
	//printf("%d\n",ret);
	int i = 8;
	if (ret == 0) {
		data = 2;
		printf("%d", ret);
		while( i != 0) {
			i --;
			printf("Child Process: Pong %d, %d;\n", data, i);
			sleep(60);
			//sleep(60);
		}
		char argv1[] = "Hello,";
		char argv2[] = "World!\n";
		char* const argv[] = {argv1, argv2, 0};

		exec(221, 20,argv);
		exit();
	}
	else if (ret != -1) {
		data = 1;
		printf("%d", ret);
		while( i != 0) {

			i --;
			printf("Father Process: Ping %d, %d;\n", data, i);
			sleep(60);
			//sleep(60);
		}
		exit();
	}

	return 0;
}
