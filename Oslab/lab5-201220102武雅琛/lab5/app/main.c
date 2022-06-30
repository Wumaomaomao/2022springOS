#include "types.h"
#include "utils.h"
#include "lib.h"

union DirEntry {
	uint8_t byte[128];
	struct {
		uint32_t inode;
		char name[64];
	};
};

typedef union DirEntry DirEntry;


int ls(char *destFilePath) {
	printf("ls %s\n", destFilePath);
	//int i = 0;
	int fd = 0;
	int ret = 0;
	DirEntry *dirEntry = 0;
	uint8_t buffer[512 * 2];
	fd = open(destFilePath, O_READ | O_DIRECTORY);
	if (fd == -1)
		return -1;
	ret = read(fd, buffer, 512 * 2);
	//printf("read ret:%d", ret);
	//printf("ret == %d!\n",ret);
	while (ret != 0) {
		// TODO: ls
		// Hint: 使用 DIrEntry
		int j;
		dirEntry = (DirEntry*)buffer;
		for (j = 0; j < 512 * 2 /  sizeof(DirEntry); j ++) {
			if (dirEntry[j].inode == 0) // a valid empty dirEntry
			{
				
				break;
			}
			else
				printf("%s ",dirEntry[j].name);

			//if (j == 512 * 2 / sizeof(DirEntry))
				//break;
		}
		if (j == 512 * 2 / sizeof(DirEntry))
			{
				ret = read(fd, buffer, 512 * 2);
				dirEntry = (DirEntry*)buffer;
			}
		else break;
	}
	printf("\n");
	close(fd);
	return 0;
}
int cat(char *destFilePath) {
	printf("cat %s\n", destFilePath);
	int fd = 0;
	int ret = 0;
	uint8_t buffer[512 * 2];
	char Printfbuffer[512 * 2 + 1];
	fd = open(destFilePath, O_READ);
	//printf("fd = %d", fd);
	if (fd == -1)
		return -1;
	ret = read(fd, buffer, 512 * 2);
	//printf("ret = %d\n", ret);
	while (ret != 0) {
		// TODO: cat
		//把内容读到buffer，输出...
		
		if (ret < 512 * 2)//读到文件末尾
		{
			int idx;
			for (idx = 0; idx < ret; ++ idx)
			{
				Printfbuffer[idx] = buffer[idx];
				//printf("char:%c\n", Printfbuffer[idx]);
			}
			Printfbuffer[idx] = '\0';
			printf("%s", Printfbuffer);
			break;//最后一个块结束循环
		}
		else
		{
			int idx;
			for (idx = 0; idx < ret; ++ idx)
			{
				Printfbuffer[idx] = buffer[idx];
			}
			Printfbuffer[idx] = '\0';
			printf("%s", Printfbuffer);
			ret = read(fd, buffer, 512 * 2);//读取下一个块
		}	
	}
	close(fd);
	return 0;
}

int uEntry(void) {
	int fd = 0;
	int i = 0;
	char tmp = 0;
	
	ls("/");
	ls("/boot/");
	ls("/dev/");
	ls("/usr/");

	printf("create /usr/test and write alphabets to it\n");
	fd = open("/usr/test", O_WRITE | O_READ | O_CREATE);
	for (i = 0; i < 26; i ++) {
		tmp = (char)(i % 26 + 'A');
		write(fd, (uint8_t*)&tmp, 1);
		//printf("test = %d", test);
	}
	close(fd);
	ls("/usr/");
	cat("/usr/test");
	printf("\n");
	printf("rm /usr/test\n");
	int ret = remove("/usr/test");
	printf("remove ret = %d\n", ret);
	ls("/usr/");
	printf("rmdir /usr/\n");
	remove("/usr");
	printf("remove ret = %d\n", ret);
	ls("/");
	printf("create /usr/\n");
	open("/usr/", O_CREATE | O_DIRECTORY);
	close(fd);
	ls("/");
	
	exit();
	return 0;
}
