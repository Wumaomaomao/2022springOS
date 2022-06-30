#include "boot.h"

#include<string.h>

#define SECTSIZE 512

#define PT_LOAD  0x1



void bootMain(void) {
	int i = 0;
	int phoff;
	//int offset = 0x1000;
	unsigned int elf = 0x200000;
	void (*kMainEntry)(void);

	for (i = 0; i < 200; i++) {
		readSect((void*)(elf + i*512), 1+i);
	}

	// TODO: 填写kMainEntry、phoff、offset...... 然后加载Kernel（可以参考NEMU的某次lab）
	ELFHeader* elfh = (ELFHeader*)elf;//elfh:the head of elf
	phoff = elfh->phoff;
	kMainEntry = (void(*)(void))(elfh->entry);

	ProgramHeader* ph, *eph;//ph:entry of programheader; eph:the next of the last ehtry of programheader
	ph = (ProgramHeader*)(elf + phoff);
	eph = ph +elfh->phnum;

	for (; ph < eph; ++ ph)
	{
		if (ph->type == PT_LOAD)
			{
				for (unsigned i = 0; i < ph->filesz; ++ i)
					*((char*)(ph->paddr + i)) = * ((char*)(elf + ph->off + i));
				//memcpy((void*)ph->paddr, (void*)(elf + ph->off), ph->filesz);
				if (ph->memsz > ph->filesz)
				{
					for (unsigned i = ph->filesz; i < ph->memsz; ++ i)
					{
						*((char*)(ph->paddr + i)) = 0;
					}
					//memset((void*)(ph->paddr + ph->filesz), 0 ,ph->memsz - ph->filesz);
				}
			}
	}

	kMainEntry();
}

void waitDisk(void) { // waiting for disk
	while((inByte(0x1F7) & 0xC0) != 0x40);
}

void readSect(void *dst, int offset) { // reading a sector of disk
	int i;
	waitDisk();
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i ++) {
		((int *)dst)[i] = inLong(0x1F0);
	}
}
