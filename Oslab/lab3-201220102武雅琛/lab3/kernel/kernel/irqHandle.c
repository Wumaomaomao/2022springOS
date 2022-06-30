#include "common.h"
#include "x86.h"
#include "device.h"

#define ARGVBUF 0x100

#define va_to_pa(va) (va + (current + 1) * 0x100000)
#define pa_to_va(pa) (pa - (current + 1) * 0x100000)

extern TSS tss;
extern int displayRow;
extern int displayCol;

extern ProcessTable pcb[MAX_PCB_NUM];
extern int current; // current process


void GProtectFaultHandle(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallExec(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);


void irqHandle(struct StackFrame *sf) { // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	
	/* Save esp to stackTop */
	//为了中断嵌套
	pcb[current].stackTop=(uint32_t)sf;

	switch(sf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(sf);
			break;
		case 0x20:
			timerHandle(sf);
			break;
		case 0x80:
			syscallHandle(sf);
			break;
		default:assert(0);
	}
}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf){
	//TODO 完成进程调度，建议使用时间片轮转，按顺序调度

	//blocked process : sleeptime decrease by 1
	if (pcb[current].state == STATE_RUNNING)// not sleep or exit
	{
		for (uint32_t idx = 1; idx < MAX_PCB_NUM; ++ idx)
		{
			if (pcb[idx].state == STATE_BLOCKED)
			{
				pcb[idx].sleepTime -= 1;
				if (pcb[idx].sleepTime <= 0)
					pcb[idx].state = STATE_RUNNABLE;
			}
		}
	}

	// timeslice expire
	if (pcb[current].timeCount >= MAX_TIME_COUNT || pcb[current].state == STATE_DEAD || pcb[current].state == STATE_BLOCKED || current == 0)
	{
		uint32_t idx;
		for (idx = 1; idx < MAX_PCB_NUM; ++ idx)
		{
			if (pcb[idx].state == STATE_RUNNABLE)
			{
				//putChar(idx + '0');
				//putChar('\n');
					putChar('s');putChar('c');putChar('h');putChar('e');putChar('d');putChar('u');putChar('l');putChar('e');putChar(':');
					putChar(current + '0');putChar('t');putChar('o');putChar(idx + '0');putChar('\n');
					if (pcb[current].state == STATE_RUNNING)
						 pcb[current].state = STATE_RUNNABLE;

					 current = idx;
					 pcb[current].state = STATE_RUNNING;
					 pcb[current].timeCount = 0;
					 uint32_t tmpStackTop=pcb[current].stackTop;
       				 tss.esp0=(uint32_t)&(pcb[current].stackTop);
        			 asm volatile("movl %0,%%esp"::"m"(tmpStackTop));
        			 asm volatile("popl %gs");
        			 asm volatile("popl %fs");
        			 asm volatile("popl %es");
        			 asm volatile("popl %ds");
        			 asm volatile("popal");
        			 asm volatile("addl $8,%esp");
        			 asm volatile("iret");

					 return;
			}
		}
		if (idx == MAX_PCB_NUM)
		{
			//putChar('D');
			//putChar('\n');
			if (pcb[current].state == STATE_RUNNING)
					pcb[current].timeCount = 0;
			else
			{
				putChar('i'); putChar('d'); putChar('l');putChar('e');putChar('\n');
				 	current = 0;
					 pcb[current].state = STATE_RUNNING;
					 pcb[current].timeCount = 0;
					 uint32_t tmpStackTop=pcb[current].stackTop;
       				 tss.esp0=(uint32_t)&(pcb[current].stackTop);
        			 asm volatile("movl %0,%%esp"::"m"(tmpStackTop));
        			 asm volatile("popl %gs");
        			 asm volatile("popl %fs");
        			 asm volatile("popl %es");
        			 asm volatile("popl %ds");
        			 asm volatile("popal");
        			 asm volatile("addl $8,%esp");
        			 asm volatile("iret");

					 return;
			}
		}
	}

	//running process: timecount increse by 1
	if (pcb[current].state == STATE_RUNNING)
	{	
		pcb[current].timeCount += 1;
	}
}


void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case 0:
			syscallWrite(sf);
			break; // for SYS_WRITE
		/* Add Fork,Sleep... */
		case 1:
			syscallFork(sf);
			break;
		case 2:
			syscallExec(sf);
			break;
		case 3:
			syscallSleep(sf);
			break;
		case 4:
			syscallExit(sf);
			break;
		default:break;
	}
}

void syscallWrite(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case 0:
			syscallPrint(sf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct StackFrame *sf) {
	int sel = sf->ds; // segment selector for user data, need further modification
	char *str = (char*)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		if(character == '\n') {
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				displayRow=24;
				displayCol=0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==80){
				displayRow++;
				displayCol=0;
				if(displayRow==25){
					displayRow=24;
					displayCol=0;
					scrollScreen();
				}
			}
		}

	}
	updateCursor(displayRow, displayCol);
	sf->eax=size;
	return;
}	

void memcpy(void* dst,void* src,size_t size){
	for(uint32_t j=0;j<size;j++){
		*(uint8_t*)(dst+j)=*(uint8_t*)(src+j);
	}
}

uint32_t strlen(char* str)
{
	uint32_t cnt = 0;
	while(*str != 0)
		{cnt ++;
			++str;
		}
	return cnt;
}

void syscallFork(struct StackFrame *sf){
	//TODO 完善它
	/*putChar('f');putChar('o');putChar('r');putChar('k');putChar(':');putChar(current + '0');putChar('\n');*/
	//TODO 查找空闲pcb，如果没有就返回-1
	int i=0;
	for (i = 1; i < MAX_PCB_NUM; ++i)
	{
		if (pcb[i].state == STATE_DEAD)
		{
			break;
		}
	}
	if (i == MAX_PCB_NUM)
	{
		sf->eax = -1;
		
		return;
	}

	//TODO 拷贝地址空间
	void* src_addr = (void*)((current+1)*0x100000);
	//void* dst_addr = src_addr + 0x100000;
	void* dst_addr =  (void*)((i+1)*0x100000);
	memcpy(dst_addr, src_addr, 0x100000);

	// 拷贝pcb，这部分代码给出了，请注意理解
	memcpy(&pcb[i],&pcb[current],sizeof(ProcessTable));
	/*for(int idx=0;idx<MAX_STACK_SIZE;idx++)
	{
			pcb[i].stack[idx] = pcb[current].stack[idx];
	}*/
	//pcb[i].s
	//putChar(i + '0');
	pcb[i].regs.eax = 0;
	pcb[i].regs.cs = USEL(1 + i * 2);
	pcb[i].regs.ds = USEL(2 + i * 2);
	pcb[i].regs.es = USEL(2 + i * 2);
	pcb[i].regs.fs = USEL(2 + i * 2);
	pcb[i].regs.gs = USEL(2 + i * 2);
	pcb[i].regs.ss = USEL(2 + i * 2);
	pcb[i].stackTop = (uint32_t)&(pcb[i].regs);
	pcb[i].state = STATE_RUNNABLE;
	pcb[i].timeCount = 0;
	pcb[i].sleepTime = 0;

	sf->eax = i;
	

}	

typedef struct 
{
	uint32_t len;
	void* addr;
	char string[20];
}argument;

void syscallExec(struct StackFrame *sf) {
	// TODO 完成exec
	// hint: 用loadelf，已经封装好了
	int sel = sf->ds;
	asm volatile("movw %0, %%es"::"m"(sel));

	uint32_t entry = 0;
	uint32_t secstart = sf->ecx;
	uint32_t secnum =  sf->edx;
	sf->esp = 0x100000;
	uint32_t endaddr = loadelf(secstart, secnum, va_to_pa(0), &entry);
	
	char** argv =(char**) va_to_pa(sf->ebx);
	uint32_t argc = 0;
	/*putChar('e');putChar('x');putChar('e');putChar('c');putChar(':s');*/
	if (pa_to_va((uint32_t)argv) != 0)//exist parameter
	{
		do{
		argv[argc]= va_to_pa(argv[argc]);//vaddr to phyaddr
		++argc;
		/*putChar('e');putChar('x');putChar('e');putChar('c');putChar(':');putChar(argc + '0');putChar('\n');*/
		}while(argv[argc] != 0);
	}
	putChar('e');putChar('x');putChar('e');putChar('c');putChar(':');
	putChar(argc + '0');putChar('\n');
	if (argc != 0)
	{
		
		/*putChar('e');putChar('x');putChar('e');putChar('c');putChar(':');putChar(argc + '0');putChar('\n');putChar('\n');*/
		uint32_t char_pointer = ARGVBUF;
		uint32_t string = ARGVBUF + (argc + 1)  * 4;
		uint32_t size = (argc + 1) * 4;
	
	for (uint32_t idx = 0; idx < argc; ++ idx)
	{	
		
		uint32_t len = strlen(argv[idx]);
		/*putChar('e');putChar('x');putChar('e');putChar('c');putChar(len + '0');putChar('\n');*/
		memcpy((void*)string, argv[idx], len + 1);
		*(uint32_t*)char_pointer = string;
		//putString(argv[idx]);
		//putString((char*)string);
	    //putString((char*)(*(uint32_t*)char_pointer));
		char_pointer += 4;
		string = string + len + 1;
		size += (len + 1);
		
	}
	
	*(uint32_t*)char_pointer = 0;
	char** p= (char**)ARGVBUF;
	
	for (uint32_t idx = 0; idx < argc; ++ idx)
	{
		p[idx] = pa_to_va(endaddr + p[idx] - ARGVBUF);
		//*(uint32_t*)(char_pointer + 4 * idx) = pa_to_va(endaddr + 4 * idx);
		//putInt(*(uint32_t*)(char_pointer + 4 * idx));
		
	}
	
	/*if (argc > 0)
	{

		argument argu[10];
		for (uint32_t idx = 0; idx < argc; ++ idx)
	{
		char* str;
		asm volatile("movl %%es:(%1), %0":"=r"(str):"r"(argv + idx)); 
		uint32_t cnt = 0;
		char temp;
		asm volatile("movb %%es:(%1), %0":"=r"(temp):"r"(str)); 
		while(temp != '\0')
		{
			argu[idx].string[cnt] = temp;
			++cnt;
			asm volatile("movb %%es:(%1), %0":"=r"(temp):"r"(str + cnt)); 
		}
		argu[idx].string[cnt] = '\0';
		argu[idx].len = cnt;

		for (int i = cnt; i >= 0; --i)
		{
			sf->esp -= 1;
			asm volatile("movb %1, %%es:(%0)":"=r"(sf->esp):"r"(argu[idx].string[cnt]));
		}
		argu[idx].addr = (char*)sf->esp;
	}
		putChar('e');putChar('x');putChar('e');putChar('c');putChar('\n');
		if (sf->esp % 4 != 0)
	{
		sf->esp = sf->esp - (sf->esp % 4);
	}

		for (uint32_t idx = argc; idx > 0; -- idx)
	{
		sf->esp -= 4;
		asm volatile("movl %1, %%es:(%0)":"=r"(sf->esp):"r"(argu[idx - 1].addr)); 
	}
		putChar('e');putChar('x');putChar('e');putChar('c');putChar('\n');
		char** pargv =(char**) sf->esp;
		sf->esp -= 4;
		asm volatile("movl %1, %%es:(%0)":"=r"(sf->esp):"r"(pargv)); 
		sf->esp -= 4;
		*((uint32_t*)va_to_pa(sf->esp)) = argc;
		sf->esp = sf->esp - 4;
	}*/
	//int p;
	//asm volatile("movl %%es:(%1), %0":"=r"(p):"r"((sf->esp)+4)); 
	/*putInt(endaddr);putChar('\n');*/
	void* bufaddr = (void*)ARGVBUF;
	memcpy((void*)endaddr, (void*)bufaddr, size);
	//sf->esp -= 4;
	*(uint32_t *)va_to_pa(sf->esp) = pa_to_va(endaddr);
	   sf->esp -= 4;
	   *(uint32_t *)va_to_pa(sf->esp) = argc;
	   sf->esp -= 4;
	}
	sf->eip = entry;
	sf->eax = 0;
	putChar('e');putChar('x');putChar('e');putChar('c');putChar('e');putChar('n');putChar('d');putChar('\n');
	return;
}


void syscallSleep(struct StackFrame *sf){
	//putChar('S');
	//putChar(sf->ecx + '0');
	//putChar('\n');
	if (sf->ecx <= 0)
	{
		sf->eax = -1;
		return;
	}
	pcb[current].timeCount = MAX_TIME_COUNT;
	pcb[current].sleepTime = sf->ecx;
	pcb[current].state = STATE_BLOCKED;
	asm volatile("int $0x20");
	//TODO:实现它
}	

void syscallExit(struct StackFrame *sf){
	//TODO 先设置成dead，然后用int 0x20进入调度
	putChar('E');putChar('x');putChar('i');putChar('t');putChar(':');putChar(current + '0');putChar('\n');
	pcb[current].state = STATE_DEAD;
	pcb[current].timeCount = MAX_TIME_COUNT;
	asm volatile("int $0x20");
}
