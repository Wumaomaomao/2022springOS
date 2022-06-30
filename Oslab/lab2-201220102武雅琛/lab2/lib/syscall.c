#include "lib.h"
#include "types.h"


/*
 * io lib here
 * 库函数写在这
 */

int32_t syscall(int num, uint32_t a1,uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
//static inline int32_t sysd,b

{
	int32_t ret = 0;
	//Generic system call: pass system call number in AX
	//up to five parameters in DX,CX,BX,DI,SI
	//Interrupt kernel with T_SYSCALL
	//
	//The "volatile" tells the assembler not to optimize
	//this instruction away just because we don't use the
	//return value
	//
	//The last clause tells the assembler that this can potentially
	//change the condition and arbitrary memory locations.

	/*
		 Note: ebp shouldn't be flushed
	    May not be necessary to store the value of eax, ebx, ecx, edx, esi, edi
	*/
	uint32_t eax, ecx, edx, ebx, esi, edi;
	uint16_t selector;
	
	asm volatile("movl %%eax, %0":"=m"(eax));
	asm volatile("movl %%ecx, %0":"=m"(ecx));
	asm volatile("movl %%edx, %0":"=m"(edx));
	asm volatile("movl %%ebx, %0":"=m"(ebx));
	asm volatile("movl %%esi,  %0":"=m"(esi));
	asm volatile("movl %%edi,  %0":"=m"(edi));
	asm volatile("movl %0, %%eax": :"m"(num));
	asm volatile("movl %0, %%ecx"::"m"(a1));
	asm volatile("movl %0, %%edx"::"m"(a2));
	asm volatile("movl %0, %%ebx"::"m"(a3));
	asm volatile("movl %0, %%esi" ::"m"(a4));
	asm volatile("movl %0, %%edi" ::"m"(a5));
	asm volatile("int $0x80");
	asm volatile("movl %%eax, %0":"=m"(ret));
	asm volatile("movl %0, %%eax"::"m"(eax));
	asm volatile("movl %0, %%ecx"::"m"(ecx));
	asm volatile("movl %0, %%edx"::"m"(edx));
	asm volatile("movl %0, %%ebx"::"m"(ebx));
	asm volatile("movl %0, %%esi"::"m"(esi));
	asm volatile("movl %0, %%edi"::"m"(edi));
	
	asm volatile("movw %%ss, %0":"=m"(selector)); //%ds is reset after iret
	//selector = 16;
	asm volatile("movw %%ax, %%ds"::"a"(selector));
	
	return ret;
}

char getChar(){ // 对应SYS_READ STD_IN
	// TODO: 实现getChar函数，方式不限
	char character = syscall(SYS_READ, STD_IN , 0, 0, 0, 0);
	return character;
}

void getStr(char *str, int size){ // 对应SYS_READ STD_STR
	// TODO: 实现getStr函数，方式不限
	uint32_t p = (size + MAX_BUFFER_SIZE - 1) / MAX_BUFFER_SIZE;
	for (uint32_t i = 0; i < p; ++ i)
	{
			syscall(SYS_READ, STD_STR,(uint32_t)(str + MAX_BUFFER_SIZE * i),size,0,0);
			uint32_t idx;
			for ( idx  = MAX_BUFFER_SIZE * i; idx <  MAX_BUFFER_SIZE * (i + 1); ++ idx)
			{
				if (str[idx] == 0)
					break;
			}
			if (idx < MAX_BUFFER_SIZE * (i + 1))
				break;
	}

	//uint32_t idx;
	/*while(1)
	{
		for (idx  = buffer_s; idx < buffer_s + MAX_BUFFER_SIZE; ++ idx)
		{
			if (str[idx] == 0)
				break;
		}
		if (idx != buffer_s + MAX_BUFFER_SIZE)
			break;
		else
		{
			buffer_s += MAX_BUFFER_SIZE;
			if (buffer_s > size)
				break;
			syscall(SYS_READ, STD_STR,(uint32_t)(str + MAX_BUFFER_SIZE),MAX_BUFFER_SIZE,0,0);
		}
	}*/
}

int dec2Str(int decimal, char *buffer, int size, int count);
int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count);
int str2Str(char *string, char *buffer, int size, int count);

void printf(const char *format,...){
	int i=0; // format index
	char buffer[MAX_BUFFER_SIZE];
	int count=0; // buffer index
	int index=0; // parameter index
	void *paraList=(void*)&format; // address of format in stack
	int state=0; // 0: legal character; 1: '%'; 2: illegal format
	int decimal=0;
	uint32_t hexadecimal=0;
	char *string=0;
	char character=0;
	//void* para=0;
	index += 4;
	while(format[i]!=0){
		//buffer[count] = format[i];
		//count++;
		//i++;
		//TODO: 可以借助状态机（回忆数电），辅助的函数已经实现好了，注意阅读手册
		if (state == 0)
		{
			if (format[i] == '%')
			{
				state = 1;
				++i;
			}
			else
			{
				state = 0;
				string[0] = format[i];
				string[1] = '\0';
				count = str2Str(string, buffer,MAX_BUFFER_SIZE,count);
				++i;
			}
		}
		else if (state == 1)
		{
			switch(format[i])
			{
				case 'd':{
					decimal = *((int*)(paraList + index));
					count = dec2Str(decimal, buffer, MAX_BUFFER_SIZE,count);
					++i;
					state = 0;
					index += 4;
					break;
				}
				case 'x':{
					hexadecimal = *((uint32_t*)(paraList + index));
					count = hex2Str(hexadecimal, buffer, MAX_BUFFER_SIZE,(uint32_t)count);
					++i;
					state = 0;
					index += 4;
					break;
				}
				case 's':{
					char** p = paraList + (uint32_t)index;
					string = (char*)(*p);
					count = str2Str(string, buffer, MAX_BUFFER_SIZE, (uint32_t)count);
					++i;
					state = 0;
					index += 4;
					break;
				}
				case 'c':{
					character = *((char*)(paraList + index));
					string[0] = character;
					string[1] = '\0';
					count = str2Str(string,buffer, MAX_BUFFER_SIZE,(uint32_t)count);
					++i;
					state = 0;
					index += 4 ;
					break;
				}
				default:state = 2;
			}
		}
		else if (state == 2)
			;
	}
	if(count!=0)
		syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)count, 0, 0);
}

int dec2Str(int decimal, char *buffer, int size, int count) {
	int i=0;
	int temp;
	int number[16];

	if(decimal<0){
		buffer[count]='-';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		temp=decimal/10;
		number[i]=temp*10-decimal;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=temp*10-decimal;
			decimal=temp;
			i++;
		}
	}
	else{
		temp=decimal/10;
		number[i]=decimal-temp*10;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=decimal-temp*10;
			decimal=temp;
			i++;
		}
	}

	while(i!=0){
		buffer[count]=number[i-1]+'0';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count) {
	int i=0;
	uint32_t temp=0;
	int number[16];

	temp=hexadecimal>>4;
	number[i]=hexadecimal-(temp<<4);
	hexadecimal=temp;
	i++;
	while(hexadecimal!=0){
		temp=hexadecimal>>4;
		number[i]=hexadecimal-(temp<<4);
		hexadecimal=temp;
		i++;
	}

	while(i!=0){
		if(number[i-1]<10)
			buffer[count]=number[i-1]+'0';
		else
			buffer[count]=number[i-1]-10+'a';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int str2Str(char *string, char *buffer, int size, int count) {
	int i=0;
	while(string[i]!=0){
		buffer[count]=string[i];
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i++;
	}
	return count;
}

void scanf(const char* format,...)
{
	void *paraList;
	paraList = &(format);
	int para = 4;
	int idx = 0;
	int state = 0;
	char buffer[MAX_BUFFER_SIZE];
	int bufp = 0;
	getStr(buffer, MAX_BUFFER_SIZE);
	while(format[idx] != 0)
	{
		if (state == 0)
		{
			if (format[idx] == '%')
				state = 1;
			++idx;
		}
		else
		{
			char  temp[MAX_BUFFER_SIZE];
			int len = 0;
			while(buffer[bufp] == ' ')
			{
				++bufp;
			}
			while(1)
			{
				if (buffer[bufp] != ' ' && buffer[bufp] != '\n' && buffer[bufp] != '\0')
				{
					temp[len] = buffer[bufp];
					++len;
					++bufp;
				}
				else break;
			}
			temp[len] = '\0';
			switch (format[idx])
			{
			case 's':
			{
				char** p = paraList + para;
				char* str = *(p);
				for (int i =0; i < len; ++ i)
				{
					str[i] = temp[i];
				}
				str[len] = '\0';
				break;
			}
				
			case 'd':
			{
				void** p = paraList + para;
				int* demical = *p;
				int value = 0;
				//printf("debug:%s\n",temp);
				int i = 0;
				while(temp[i] != 0x0)
				{
					value = value * 10 + temp[i] - '0' ;
					++i;
				}

				*demical = value;
				break;
			}
			default:
				break;
			}
			
			state = 0;
			++idx;
			para += 4;
		}
	}
}