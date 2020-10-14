#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];
TSS tss;

#define SECTSIZE 512

void waitDisk(void) {
	while((inByte(0x1F7) & 0xC0) != 0x40);
}

void readSect(void *dst, int offset) {
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

void initSeg() {
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_USER);
	gdt[3] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	gdt[SEG_UDATA] = SEG(STA_W,         0,       0xffffffff, DPL_USER);
	gdt[SEG_TSS] = SEG16(STS_T32A,   &tss,    sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	gdt[7] = SEG(STA_W,  0x0b8000,       0xffffffff, DPL_KERN);
	setGdt(gdt, sizeof(gdt));


	tss.esp0 = (uint32_t)&pcb[0].stack[KERNEL_STACK_SIZE];
	tss.ss0  = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));
	/* set kernel segment register */
	asm volatile("movl %0, %%eax":: "r"(KSEL(SEG_KDATA)));
	asm volatile("movw %ax, %ds");
	asm volatile("movw %ax, %es");
	asm volatile("movw %ax, %ss");
	asm volatile("movw %ax, %fs");
	asm volatile("movl %0, %%eax":: "r"(KSEL(SEG_VIDEO)));
	asm volatile("movw %ax, %gs");
	//asm volatile('nop');
	//putChar('1');
	lLdt(0);

}
void enterUserSpace(uint32_t entry) {
	asm volatile("movl %0, %%eax":: "r"(USEL(SEG_UDATA)));
	asm volatile("movw %ax, %ds");
	asm volatile("movw %ax, %es");
	asm volatile("movw %ax, %fs");
	asm volatile("sti");
	asm volatile("pushl %0":: "r"(USEL(SEG_UDATA)));	// %ss
	asm volatile("pushl %0":: "r"(64 << 20));			// %esp
	asm volatile("pushfl");								
	asm volatile("pushl %0":: "r"(USEL(SEG_UCODE)));	
	asm volatile("pushl %0":: "r"(entry));				

	asm volatile("iret"); // return to user space
}
void loadUMain(void) {
	struct ELFHeader *elf;
	struct ProgramHeader *program_header;
	unsigned char temp[10000];
	//unsigned cha
	for (int i = 0; i < 100; i ++) {
		readSect((void*)(temp + (256*2) * i), i + 200+1);
	}

	elf = (struct ELFHeader *)temp;
	//asm volatile("pushl %0" :: "r"(USEL(SEG_UDATA)));
	//asm volatile("pushfl");
	//use iret to jump to ring3
	//asm volatile("iret"):
	//asm voaltile("nop");
	//asm volatile("incl %ss");
	
	program_header = (struct ProgramHeader *)(temp + elf->phoff);
	int i;
	for(i = 0; i < elf->phnum; ++i) {
	
		asm volatile("nop");
		if (program_header->type == 1) {
			unsigned int p = program_header->vaddr, q = program_header->off;
			while (p < program_header->vaddr + program_header->filesz) {
				*(unsigned char*)p = *(unsigned char*)(temp + q);
				q++;
				p++;
			}
		//The number of bits in a bit fiedl sets the limit to the range of values it can hold
		//Refer: en.cppreference.com/w/cpp/language/bit_field
		//..
			
			while (p < program_header->vaddr + program_header->memsz) {
				*(unsigned char*)p = 0;
				q++;
				p++;
			}
		}

		program_header++;
	}
	//low bits of segment limit and base address
	//0-system, 1= application

    int m = 0x200000;
    int n = 0x300000;
    for (int i = 0; i < 0x100000; i++) {
        *((uint8_t *)n + i) = *((uint8_t *)m + i);
    }
	asm volatile("nop");
	enterIDLE(elf->entry);
	
}
