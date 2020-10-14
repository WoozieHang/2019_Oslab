#include "boot.h"

#define SECTSIZE 512

void bootMain(void) {
	/* 加载内核至内存，并跳转执行 */
        struct ELFHeader *elf;
        struct ProgramHeader *ph;
	unsigned char *buf=(unsigned char *)0x900000;//load from internal to buf
	for(int i=0;i<200;i++)
		readSect((void*)(buf + 512 * i) , i + 1);
        asm volatile("nop");
 	elf= ( struct ELFHeader *)buf;
	ph = ( struct ProgramHeader *)(buf + elf->phoff);
	for(int i = 0; i < elf->phnum; ++i) {

		/*  load each segment into memory */
		if (ph->type == 1) { // LOAD Section

			/*  from the ELF file
			  to the memory region [VirtAddr, VirtAddr + FileSiz)
			 */
			unsigned int p = ph->vaddr, q = ph->off;
			while (p < ph->vaddr + ph->filesz) {
				*(unsigned char*)p = *(unsigned char*)(buf + q);
				q++;
				p++;
			}

			/* zero the memory region [VirtAddr + FileSiz, VirtAddr + MemSiz) */
			while (p < ph->vaddr + ph->memsz) {
				*(unsigned char*)p = 0;
				q++;
				p++;
			}
		}

		ph++;
	}

	/* jmp to kernel */
	void (*entry)(void);
	entry = (void*)(elf->entry);
	entry();
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
