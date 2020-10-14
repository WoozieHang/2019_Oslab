#ifndef __X86_PCB_H_
#define __X86_PCB_H_

#include <common.h>
#include "memory.h"

#define KERNEL_STACK_SIZE (16 << 10)
#define PROCESS_MEMORY_SIZE (1<<16)
#define USER_ENTRY 0x200000
#define MAX_PCB 30
#define TIME_UNIT 10
#define PID_OFFSET 1000
#define IDLE_STACK 0x200000
#define BLOCKED 0
#define DEAD 1
#define RUNNING 2
#define RUNNABLE 3

struct ProcessBlock{
	union{
		uint8_t stack[KERNEL_STACK_SIZE];
		struct{
			uint8_t pad[KERNEL_STACK_SIZE - sizeof(struct TrapFrame)];
			struct TrapFrame tf;
		}__attribute__((packed));
	};
	int state;
	int timeCount;
	int sleepTime;
	uint32_t pid;
	struct ProcessBlock *next;
};
struct ProcessBlock pcb[MAX_PCB];
struct ProcessBlock* pcb_head;
struct ProcessBlock* pcb_free;
struct ProcessBlock* pcb_cur;

#define pcb_number(p) (p-pcb)
void initPCB();
void enterIDLE(uint32_t entry);
void scheduler();
struct ProcessBlock* getPCB();

#endif

