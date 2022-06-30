#include "x86.h"
#include "device.h"
#include "fs.h"

#define SYS_OPEN 0
#define SYS_WRITE 1
#define SYS_READ 2
#define SYS_LSEEK 3
#define SYS_CLOSE 4
#define SYS_REMOVE 5
#define SYS_FORK 6
#define SYS_EXEC 7
#define SYS_SLEEP 8
#define SYS_EXIT 9
#define SYS_SEM 10

#define STD_OUT 0
#define STD_IN 1

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

#define UMASK 0x0022

extern TSS tss;

extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern Semaphore sem[MAX_SEM_NUM];
extern Device dev[MAX_DEV_NUM];
extern File file[MAX_FILE_NUM];

extern SuperBlock sBlock;
extern GroupDesc gDesc[MAX_GROUP_NUM];

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void GProtectFaultHandle(struct StackFrame *sf);
void timerHandle(struct StackFrame *sf);
void keyboardHandle(struct StackFrame *sf);
void syscallHandle(struct StackFrame *sf);

void syscallOpen(struct StackFrame *sf);
void syscallWrite(struct StackFrame *sf);
void syscallRead(struct StackFrame *sf);
void syscallLseek(struct StackFrame *sf);
void syscallClose(struct StackFrame *sf);
void syscallRemove(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallExec(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);
void syscallSem(struct StackFrame *sf);

void syscallWriteStdOut(struct StackFrame *sf);
void syscallWriteFile(struct StackFrame *sf);

void syscallReadStdIn(struct StackFrame *sf);
void syscallReadFile(struct StackFrame *sf);

void syscallSemInit(struct StackFrame *sf);
void syscallSemWait(struct StackFrame *sf);
void syscallSemPost(struct StackFrame *sf);
void syscallSemDestroy(struct StackFrame *sf);

void irqHandle(struct StackFrame *sf) { // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	/* Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch(sf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(sf);
			break;
		case 0x20:
			timerHandle(sf);
			break;
		case 0x21:
			keyboardHandle(sf);
			break;
		case 0x80:
			syscallHandle(sf);
			break;
		default:assert(0);
	}
	/* Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf) {
	int i;
	uint32_t tmpStackTop;
	i = (current+1) % MAX_PCB_NUM;
	while (i != current) {
		if (pcb[i].state == STATE_BLOCKED && pcb[i].sleepTime != -1) {
			pcb[i].sleepTime --;
			if (pcb[i].sleepTime == 0)
				pcb[i].state = STATE_RUNNABLE;
		}
		i = (i+1) % MAX_PCB_NUM;
	}

	if (pcb[current].state == STATE_RUNNING &&
		pcb[current].timeCount != MAX_TIME_COUNT) {
		pcb[current].timeCount++;
		return;
	}
	else {
		if (pcb[current].state == STATE_RUNNING) {
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}
		
		i = (current+1) % MAX_PCB_NUM;
		while (i != current) {
			if (i !=0 && pcb[i].state == STATE_RUNNABLE)
				break;
			i = (i+1) % MAX_PCB_NUM;
		}
		if (pcb[i].state != STATE_RUNNABLE)
			i = 0;
		current = i;
		/* echo pid of selected process */
		//putChar('0'+current);
		pcb[current].state = STATE_RUNNING;
		pcb[current].timeCount = 1;
		/* recover stackTop of selected process */
		tmpStackTop = pcb[current].stackTop;
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t)&(pcb[current].stackTop); // setting tss for user process
		asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
}

void keyboardHandle(struct StackFrame *sf) {
	ProcessTable *pt = NULL;
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0) // illegal keyCode
		return;
	//putChar(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail=(bufferTail+1)%MAX_KEYBUFFER_SIZE;
	if (dev[STD_IN].value < 0) { // with process blocked
		dev[STD_IN].value ++;

		pt = (ProcessTable*)((uint32_t)(dev[STD_IN].pcb.prev) -
					(uint32_t)&(((ProcessTable*)0)->blocked));
		pt->state = STATE_RUNNABLE;
		pt->sleepTime = 0;

		dev[STD_IN].pcb.prev = (dev[STD_IN].pcb.prev)->prev;
		(dev[STD_IN].pcb.prev)->next = &(dev[STD_IN].pcb);
	}
	return;
}

void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case SYS_OPEN:
			syscallOpen(sf);
			break; // for SYS_OPEN
		case SYS_WRITE:
			syscallWrite(sf);
			break; // for SYS_WRITE
		case SYS_READ:
			syscallRead(sf);
			break; // for SYS_READ
		case SYS_LSEEK:
			syscallLseek(sf);
			break; // for SYS_SEEK
		case SYS_CLOSE:
			syscallClose(sf);
			break; // for SYS_CLOSE
		case SYS_REMOVE:
			syscallRemove(sf);
			break; // for SYS_REMOVE
		case SYS_FORK:
			syscallFork(sf);
			break; // for SYS_FORK
		case SYS_EXEC:
			syscallExec(sf);
			break; // for SYS_EXEC
		case SYS_SLEEP:
			syscallSleep(sf);
			break; // for SYS_SLEEP
		case SYS_EXIT:
			syscallExit(sf);
			break; // for SYS_EXIT
		case SYS_SEM:
			syscallSem(sf);
			break; // for SYS_SEM
		default:break;
	}
}

void syscallOpen(struct StackFrame *sf) {
	int i;

	int ret = 0;
	//int size = 0;
	int baseAddr = (current + 1) * 0x100000; // base address of user process
	char *str = (char*)sf->ecx + baseAddr; // file path
	Inode fatherInode;
	Inode destInode;
	int fatherInodeOffset = 0;
	int destInodeOffset = 0;
	uint32_t flags = sf->edx;

	ret = readInode(&sBlock, gDesc, &destInode, &destInodeOffset, str);

	if (ret == 0) { // file exist
		// TODO: Open1
		// 错误处理，在目标文件存在的条件下，flags设置的类型与该文件实际类型不一致，返回-1
		if ((flags & 0x8) == 0x8)// open directory
		{
			if ((destInode.type & 0x7) != DIRECTORY_TYPE)//如果已存在的文件并非目录文件
				{
					sf->eax = -1;
					return;
				}

		}
		/*if (flags & 0x1 == 0x1)//open file with writing
		{
			if (destInode.type & 0x10 != 0x10)//如果用户无写权限
				{	
					sf->eax = -1;
					return;
				}
		}
		if (flags & 0x2 == 0x2)//open file with reading
		{
			if (destInode.type & 0x20 != 0x20)//如果用户无读权限
				{
					sf->eax = -1;
					return;
				}
		}*/

		//TODO: Open2
		// 错误处理，判断是否已经被打开，如果已经被打开就返回-1（遍历dev和file数组）
		for(i = 0; i < MAX_FILE_NUM; i++)
		{
			if (file[i].state == 1 && file[i].inodeOffset == destInodeOffset)
				{
					sf->eax = -1;
					return;
				}
		}
		

		//if there is no error, open the file
		for(i=0;i<MAX_FILE_NUM;i++){
			if(file[i].state==0){
				//putChar(i+'0');
				file[i].state = 1;
				file[i].inodeOffset = destInodeOffset;
				file[i].offset = 0;
				file[i].flags = sf->edx;
				pcb[current].regs.eax = MAX_DEV_NUM + i;
				return;
			}
		}
		//if no free FCB, return error
		if(i==MAX_FILE_NUM){
			sf->eax=-1;
			return;
		}

	}
	else { // try to create file

		//TODO: Open3
		//错误处理，不存在这个文件，并且O_CREATE没有被设置（O_CREATE如何判断，参考上面或者下面）
		if ((flags & 0x4) != 0x4)
		{
			sf->eax = -1;
			return;
		}

		if ((sf->edx >> 3) % 2 == 0) { 
			//TODO: Open4        
			// 到了这里，目标文件不存在，并且CREATE位设置为1，并且要创建的目标文件是一个常规文件
			// Hint: readInode allocInode
			//获得父亲目录路径
			int pos = -1;
			stringChrR(str,'/',&pos);
			char fatherstr[128]; 
			char filename[128];
			stringCpy(str + pos + 1, filename, stringLen(str) - pos - 1);
			stringCpy(str, fatherstr, pos);
			//获取父亲目录文件的Inode节点和inode号
			ret = readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, fatherstr);
			//设置inode节点文件类型
			uint16_t type = 0;
			type = type | REGULAR_TYPE;
			allocInode(&sBlock,gDesc,&fatherInode,fatherInodeOffset,&destInode,&destInodeOffset,filename,type);
			//ret = readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, str);
		}
		else { 
			//TODO: Open5        
			// 目标文件不存在，并且CREATE位设置为1，并且要创建的目标文件是一个目录文件
			// Hint: readInode allocInode
			int pos = -1;
			stringChrR(str,'/',&pos);
			char fatherstr[128]; 
			stringCpy(str, fatherstr, pos);
			//获取父亲目录文件的Inode节点和inode号
			ret = readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, fatherstr);
			//设置inode节点文件类型
			uint16_t type = 0;
			type = type | DIRECTORY_TYPE;
			allocInode(&sBlock,gDesc,&fatherInode,fatherInodeOffset,&destInode,&destInodeOffset,fatherstr,type);
		}

		//分配完inode，要打开，存入file数组
		for (i = 0; i < MAX_FILE_NUM; i++) {
			if (file[i].state == 0) { // not in use
				file[i].state = 1;
				file[i].inodeOffset = destInodeOffset;
				file[i].offset = 0;
				file[i].flags = sf->edx;
				pcb[current].regs.eax = MAX_DEV_NUM + i;
				return;
			}
		}
		if(i==MAX_FILE_NUM){
			pcb[current].regs.eax = -1; // create success but no available file[]
			return;
		}
	}
}

void syscallWrite(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case STD_OUT:
			if (dev[STD_OUT].state == 1)
				syscallWriteStdOut(sf);
			break; // for STD_OUT
		default:break;
	}

	// TODO: Write1        
	// 如果要向文件里写入，在这里进行错误处理：超出文件范围或者该文件没有打开，返回-1
	int fd = sf->ecx;
	if ((fd - MAX_DEV_NUM) < 0 && (fd - MAX_DEV_NUM) > MAX_FILE_NUM)
	{
		sf->eax = -1;
		return;
	}
	if (file[fd - MAX_DEV_NUM].state == 0)
	{
		sf->eax = -1;
		return;
	}
	
	
	syscallWriteFile(sf);
	return;
}

void syscallWriteStdOut(struct StackFrame *sf) {
	int sel = sf->ds; 
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
			if(displayRow==MAX_ROW){
				displayRow=MAX_ROW-1;
				displayCol=0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (MAX_COL*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==MAX_COL){
				displayRow++;
				displayCol=0;
				if(displayRow==MAX_ROW){
					displayRow=MAX_ROW-1;
					displayCol=0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	pcb[current].regs.eax = size;
	return;
}

void syscallWriteFile(struct StackFrame *sf) {
	if (file[sf->ecx - MAX_DEV_NUM].flags % 2 == 0) { // if O_WRITE is not set
		pcb[current].regs.eax = -1;
		return;
	}

	//int j = 0;
	int baseAddr = (current + 1) * 0x100000; // base address of user process
	uint8_t *str = (uint8_t*)sf->edx + baseAddr; // buffer of user process
	int size = sf->ebx;
	uint8_t buffer[SECTOR_SIZE * SECTORS_PER_BLOCK];
	int quotient = file[sf->ecx - MAX_DEV_NUM].offset / sBlock.blockSize;
	int remainder = file[sf->ecx - MAX_DEV_NUM].offset % sBlock.blockSize;

	if(size<=0){
		sf->eax=0;
		return;
	}

	//先读出inode
	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[sf->ecx - MAX_DEV_NUM].inodeOffset);
	

	int stroff=0;

	int sz=size;
	// TODO: WriteFile1
	// Hint: 
	// 使用 readBlock 来读出内容到 buffer
	// 使用MemCpy在内存操纵 buffer ，把内容写入 buffer
	// 使用 writeBlock 把 buffer 的内容写回数据
	// 这个比较麻烦，要分清楚 quotient、remainder、j 这些都是什么
	// 
	int blockcnt = 0;
	if (quotient < inode.blockCount)//如果已经创建了块
	{
		readBlock(&sBlock,&inode,quotient,buffer);
	}
	else
	{
		allocBlock(&sBlock,gDesc,&inode,file[sf->ecx - MAX_DEV_NUM].inodeOffset);
	}
	if (size <= sBlock.blockSize - remainder)//如果没有发生跨块写入
	{
		MemCpy(str, buffer + remainder, size);
		writeBlock(&sBlock,&inode,quotient,buffer);
		stroff += size;
	}
	else//发生跨块写入
	{
		MemCpy(str, buffer + remainder, sBlock.blockSize - remainder);
		writeBlock(&sBlock,&inode,quotient,buffer);
		stroff += sBlock.blockSize - remainder;
		blockcnt ++;
		while(stroff < size)
		{
			if (quotient + blockcnt < inode.blockCount)
				allocBlock(&sBlock,gDesc,&inode,file[sf->ecx - MAX_DEV_NUM].inodeOffset);
			else
				readBlock(&sBlock,&inode,quotient + blockcnt,buffer);
			if (size - stroff <= sBlock.blockSize)//如果访问到最后一个逻辑块
			{
				MemCpy(str + blockcnt * sBlock.blockSize, buffer, size - stroff);
				writeBlock(&sBlock,&inode,quotient + blockcnt,buffer);
				stroff += size - stroff;
			}
			else
			{
				MemCpy(str + blockcnt * sBlock.blockSize, buffer, sBlock.blockSize);
				writeBlock(&sBlock,&inode,quotient + blockcnt,buffer);
				stroff += sBlock.blockSize;
			}
			++blockcnt;
		}
	}
	

	
	// TODO: WriteFile2
	// 这里把inode修改后写回磁盘（inode的size需要修改）
	// 使用 diskWrite 函数
	inode.size += stroff;
	//putString("After write:inode.size =");putInt(inode.size);

	diskWrite((void*)&inode, sizeof(Inode), 1, file[sf->ecx - MAX_DEV_NUM].inodeOffset);
	
	pcb[current].regs.eax = sz;
	file[sf->ecx - MAX_DEV_NUM].offset += sz;
	return;
}

void syscallRead(struct StackFrame *sf) {
	switch(sf->ecx) { // file descriptor
		case STD_IN:
			if (dev[STD_IN].state == 1)
				syscallReadStdIn(sf);
			break; // for STD_IN
		default:break;
	}
	// TODO: Read1         
	// 读取文件，在这里进行错误处理：超出文件范围或者该文件没有打开，返回-1
	int fd = sf->ecx;
	if ((fd - MAX_DEV_NUM) < 0 && (fd - MAX_DEV_NUM) > MAX_FILE_NUM)
	{
		sf->eax = -1;
		return;
	}
	if (file[fd - MAX_DEV_NUM].state == 0)
	{
		sf->eax = -1;
		return;
	}

	syscallReadFile(sf);
	return;

}

void syscallReadStdIn(struct StackFrame *sf) {
	if (dev[STD_IN].value == 0) { // no process blocked
		/* Blocked for I/O */
		dev[STD_IN].value --;

		pcb[current].blocked.next = dev[STD_IN].pcb.next;
		pcb[current].blocked.prev = &(dev[STD_IN].pcb);
		dev[STD_IN].pcb.next = &(pcb[current].blocked);
		(pcb[current].blocked.next)->prev = &(pcb[current].blocked);

		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = -1; // blocked on STD_IN

		bufferHead = bufferTail;
		asm volatile("int $0x20");
		/* Resumed from Blocked */
		int sel = sf->ds;
		char *str = (char*)sf->edx;
		int size = sf->ebx; // MAX_BUFFER_SIZE, reverse last byte
		int i = 0;
		char character = 0;
		asm volatile("movw %0, %%es"::"m"(sel));
		while(i < size-1) {
			if(bufferHead != bufferTail){ // what if keyBuffer is overflow
				character = getChar(keyBuffer[bufferHead]);
				bufferHead = (bufferHead+1)%MAX_KEYBUFFER_SIZE;
				putChar(character);
				if(character != 0) {
					asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(str+i));
					i++;
				}
			}
			else
				break;
		}
		asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
		pcb[current].regs.eax = i;
		return;
	}
	else if (dev[STD_IN].value < 0) { // with process blocked
		pcb[current].regs.eax = -1;
		return;
	}
}

void syscallReadFile(struct StackFrame *sf) {
	if ((file[sf->ecx - MAX_DEV_NUM].flags >> 1) % 2 == 0) { // if O_READ is not set
		pcb[current].regs.eax = -1;
		return;
	}

	//int i = 0;
	//int j = 0;
	int baseAddr = (current + 1) * 0x100000; // base address of user process
	uint8_t *str = (uint8_t*)sf->edx + baseAddr; // buffer of user process
	int size = sf->ebx; // MAX_BUFFER_SIZE, don't need to reserve last byte
	uint8_t buffer[SECTOR_SIZE * SECTORS_PER_BLOCK];
	int quotient = file[sf->ecx - MAX_DEV_NUM].offset / sBlock.blockSize;
	int remainder = file[sf->ecx - MAX_DEV_NUM].offset % sBlock.blockSize;


	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[sf->ecx - MAX_DEV_NUM].inodeOffset);
	
	if(size<=0){
		sf->eax=0;
		return;
	}
	

	int stroff=0;
	int FCBindex = sf->ecx - MAX_DEV_NUM;
	if(size + file[FCBindex].offset > inode.size){
		//超出文件大小，就把size进行调整
		size = inode.size - file[FCBindex].offset;
	}
	int sz=size;
	// TODO: ReadFile1  
	// 提示，使用readBlock和MemCpy，逐个block读
	// readBlock已经封装好了读取操作。参数：超级块，inode，块索引，buffer
 	// 注意理解上面的quotient、remainder、size和j都是啥玩意儿
	int blockcnt = 0;
	if (size <= sBlock.blockSize - remainder)//如果没有发生跨块读出
	{
		readBlock(&sBlock, &inode,quotient,buffer);
		MemCpy(buffer + remainder, str, size);
		
	}
	else
	{
		readBlock(&sBlock, &inode,quotient,buffer);
		MemCpy(buffer + remainder, str, sBlock.blockSize - remainder);
		stroff += sBlock.blockSize - remainder;
		++blockcnt;
		while(stroff < size)
		{
			
			if (size - stroff <= sBlock.blockSize)//如果访问到最后一个逻辑块
			{
				readBlock(&sBlock,&inode,quotient + blockcnt,buffer);
				MemCpy(buffer, str + stroff, size - stroff);
				stroff += size - stroff;
			}
			else
			{
				readBlock(&sBlock,&inode,quotient + blockcnt,buffer);
				MemCpy(buffer, str + stroff, sBlock.blockSize);
				stroff += sBlock.blockSize;
			}
			++blockcnt;
		}
	}




	pcb[current].regs.eax = sz;
	file[sf->ecx - MAX_DEV_NUM].offset += sz;
	return;
}

void syscallLseek(struct StackFrame *sf) {
	int offset = (int)sf->edx;
	Inode inode;
	
	int FCBindex = (int)sf -> ecx-MAX_DEV_NUM;
	if (FCBindex < 0 || FCBindex >=  MAX_FILE_NUM || file[FCBindex].state == 0) {//out of range
		pcb[current].regs.eax = -1;
		return;
	}
	diskRead(&inode, sizeof(Inode), 1, file[FCBindex].inodeOffset);
	//INFO: 下面设置ofs
	int ofs=0;
	switch(sf->ebx) { // whence
		case SEEK_SET:
			// TODO: Lseek1        
			ofs = offset;
			break;
		case SEEK_CUR:
			// TODO: Lseek2
			ofs = file[FCBindex].offset + offset;
			break;
		case SEEK_END:
			// TODO: Lseek3
			ofs = inode.size + offset;
			break;
		default:
			break;
	}
	if(ofs<0||ofs>=inode.size){
		sf->eax=-1;
		return;
	}
	file[FCBindex].offset = ofs;
	sf->eax=0;
	return;
}

void syscallClose(struct StackFrame *sf) {
	int i = (int)sf->ecx;
	if (i < MAX_DEV_NUM || i >= MAX_DEV_NUM + MAX_FILE_NUM) { 
		// TODO: Close1        
		// 错误，设备是不能被关闭的，或者数组越界（超过DEV和FILE数目之和），返回-1
		sf->eax = -1;
		return;
	}
	if (file[i - MAX_DEV_NUM].state == 0) { 
		// TODO: Close2    
		// 错误，文件根本就没有打开，返回-1
		sf->eax = -1;
		return;
	}
	//TODO: Close3
	// 关闭文件，对file数组操作
	file[i - MAX_DEV_NUM].state = 0;
	
	
	pcb[current].regs.eax = 0;
	return;
}

void syscallRemove(struct StackFrame *sf) {

	int ret = 0;
	//int size = 0;
	int baseAddr = (current + 1) * 0x100000; // base address of user process
	char *str = (char*)sf->ecx + baseAddr; // file path
	Inode fatherInode;
	Inode destInode;
	int fatherInodeOffset = 0;
	int destInodeOffset = 0;

	ret = readInode(&sBlock, gDesc, &destInode, &destInodeOffset, str);
	if (ret == 0) { // file exist
		// TODO: Remove1   
		// 错误处理，考虑该文件或者它引用的文件或设备被使用的情况，这时返回-1退出
		// 为了简化，只考虑文件本身正在被使用的情况（注意inode里面的成员，linkCount）
		//destInode.linkCount -= 1;
		for (int i = 0; i < MAX_FILE_NUM; i++) {
			if (file[i].state == 1 && file[i].inodeOffset == destInodeOffset)
				{
					sf->eax = -1;
					return;
				}
		}

		// free inode
		if (destInode.type == REGULAR_TYPE) {
			// TODO: Remove2   
			// 如果是常规文件，删除
			// 注意错误处理，比如Type是常规文件，但路径是个目录...
			// Hint: 用readInode，freeInode
			int pos = -1;
			stringChrR(str,'/',&pos);
			char fatherstr[128]; 
			char filename[128];
			stringCpy(str + pos + 1, filename, stringLen(str) - pos -1);
			stringCpy(str, fatherstr, pos); 
			ret = readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, fatherstr);
			if (ret == -1)//目录不存在
			{
				sf->eax = -1;
				return;
			}
			uint16_t type = 0;
			type = type | REGULAR_TYPE;
			freeInode(&sBlock,gDesc,&fatherInode,fatherInodeOffset,&destInode,&destInodeOffset,filename,type);
			
		}
		else if (destInode.type == DIRECTORY_TYPE) {
			// TODO: Remove3   
			// 如果是目录，删除
			// Hint: 用readInode， freeInode
			
			int pos = -1;
			stringChrR(str,'/',&pos);
			char fatherstr[128]; 
			char filename[128];
			stringCpy(str + pos + 1, filename, stringLen(str) - pos - 1);
			//putString(filename);
			stringCpy(str, fatherstr, pos + 1); 
			//putString(fatherstr);
			ret = readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, fatherstr);
			if (ret == -1)//目录不存在
			{
				//putString("Dic not exist!\n");
				sf->eax = -1;
				return;
			}
			uint16_t type = 0;
			type = type | DIRECTORY_TYPE;
			freeInode(&sBlock,gDesc,&fatherInode,fatherInodeOffset,&destInode,&destInodeOffset,filename,type);

		}
		if (ret == -1) {
			pcb[current].regs.eax = -1;
			return;
		}
		pcb[current].regs.eax = 0;
		return;
	}
	else { // file not exist
		pcb[current].regs.eax = -1;
		return;
	}
}

void syscallFork(struct StackFrame *sf) {
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD)
			break;
	}
	if (i != MAX_PCB_NUM) {
		/* copy userspace
		   enable interrupt
		 */
		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (i+1)*0x100000) = *(uint8_t *)(j + (current+1)*0x100000);
			//asm volatile("int $0x20"); // Testing irqTimer during syscall
		}
		/* disable interrupt
		 */
		disableInterrupt();
		/* set pcb
		   pcb[i]=pcb[current] doesn't work
		*/
		pcb[i].stackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].stackTop);
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].prevStackTop);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].timeCount = pcb[current].timeCount;
		pcb[i].sleepTime = pcb[current].sleepTime;
		pcb[i].pid = i;
		/* set regs */
		pcb[i].regs.ss = USEL(2+i*2);
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.cs = USEL(1+i*2);
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eax = pcb[current].regs.eax;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.xxx = pcb[current].regs.xxx;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.ds = USEL(2+i*2);
		pcb[i].regs.es = pcb[current].regs.es;
		pcb[i].regs.fs = pcb[current].regs.fs;
		pcb[i].regs.gs = pcb[current].regs.gs;
		/* set return value */
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallExec(struct StackFrame *sf) {
	return;
}

void syscallSleep(struct StackFrame *sf) {
	if (sf->ecx == 0)
		return;
	else {
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = sf->ecx;
		asm volatile("int $0x20");
		return;
	}
}

void syscallExit(struct StackFrame *sf) {
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}

void syscallSem(struct StackFrame *sf) {
	switch(sf->ecx) {
		case SEM_INIT:
			syscallSemInit(sf);
			break;
		case SEM_WAIT:
			syscallSemWait(sf);
			break;
		case SEM_POST:
			syscallSemPost(sf);
			break;
		case SEM_DESTROY:
			syscallSemDestroy(sf);
			break;
		default:break;
	}
}

void syscallSemInit(struct StackFrame *sf) {
	int i;
	for (i = 0; i < MAX_SEM_NUM ; i++) {
		if (sem[i].state == 0) // not in use
			break;
	}
	if (i != MAX_SEM_NUM) {
		sem[i].state = 1;
		sem[i].value = (int32_t)sf->edx;
		sem[i].pcb.next = &(sem[i].pcb);
		sem[i].pcb.prev = &(sem[i].pcb);
		pcb[current].regs.eax = i;
	}
	else
		pcb[current].regs.eax = -1;
	return;
}

void syscallSemWait(struct StackFrame *sf) {
	int i = (int)sf->edx;
	if (i < 0 || i >= MAX_SEM_NUM) {
		pcb[current].regs.eax = -1;
		return;
	}
	if (sem[i].state == 0) { // not in use
		pcb[current].regs.eax = -1;
		return;
	}
	if (sem[i].value >= 1) { // not to block itself
		sem[i].value --;
		pcb[current].regs.eax = 0;
		return;
	}
	if (sem[i].value < 1) { // block itself on this sem
		sem[i].value --;
		pcb[current].blocked.next = sem[i].pcb.next;
		pcb[current].blocked.prev = &(sem[i].pcb);
		sem[i].pcb.next = &(pcb[current].blocked);
		(pcb[current].blocked.next)->prev = &(pcb[current].blocked);
		
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = -1;
		asm volatile("int $0x20");
		pcb[current].regs.eax = 0;
		return;
	}
}

void syscallSemPost(struct StackFrame *sf) {
	int i = (int)sf->edx;
	ProcessTable *pt = NULL;
	if (i < 0 || i >= MAX_SEM_NUM) {
		pcb[current].regs.eax = -1;
		return;
	}
	if (sem[i].state == 0) { // not in use
		pcb[current].regs.eax = -1;
		return;
	}
	if (sem[i].value >= 0) { // no process blocked
		sem[i].value ++;
		pcb[current].regs.eax = 0;
		return;
	}
	if (sem[i].value < 0) { // release process blocked on this sem 
		sem[i].value ++;

		pt = (ProcessTable*)((uint32_t)(sem[i].pcb.prev) -
					(uint32_t)&(((ProcessTable*)0)->blocked));
		pt->state = STATE_RUNNABLE;
		pt->sleepTime = 0;

		sem[i].pcb.prev = (sem[i].pcb.prev)->prev;
		(sem[i].pcb.prev)->next = &(sem[i].pcb);
		
		pcb[current].regs.eax = 0;
		return;
	}
}


void syscallSemDestroy(struct StackFrame *sf) {
	int i = (int)sf->edx;
	if (i < 0 || i >= MAX_SEM_NUM) {
		pcb[current].regs.eax = -1;
		return;
	}
	if (sem[i].state == 0) { // not in use
		pcb[current].regs.eax = -1;
		return;
	}
	sem[i].state = 0;
	sem[i].value = 0;
	sem[i].pcb.next = &(sem[i].pcb);
	sem[i].pcb.prev = &(sem[i].pcb);
	pcb[current].regs.eax = 0;
	return;
}
