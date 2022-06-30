#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		printf("Please execute the program with a CLI argument!\n");
	}
	else if (argc > 2)
	{
		printf("Too many arguments!!!\n");
	}
	else
	{
		FILE* fp;
		fp = fopen(argv[1], "rb +");
		fseek(fp, 0, SEEK_END);
		int  flen = ftell(fp);
		//printf("%d\n", flen);

		if (flen > 510)
			printf("Error, the size of file must smaller than 510!!");
		else
		{
			unsigned nulllen = 510 - flen;

			char* buf = (char*)malloc(nulllen + 2);

			memset(buf, 0, nulllen);

			buf[nulllen] = 0x55;
			buf[nulllen + 1] = 0xAA;

			fwrite(buf, 1, nulllen + 2, fp);

			fclose(fp);
		}

	}
}