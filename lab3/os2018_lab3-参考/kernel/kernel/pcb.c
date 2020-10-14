#include "device.h"
#include "x86.h"

extern SegDesc gdt[NR_SEGMENTS];
extern TSS tss;

void IDLE() {
    asm volatile("movl %0, %%esp;" ::"i"(IDLE_STACK));
    asm volatile("sti");
    waitForInterrupt();
}

void scheduler() {	
    struct ProcessBlock *p;

  	asm volatile("nop");  
    if (pcb_cur != NULL) {
        p = pcb_cur->next;
        if (p != NULL) {
            pcb_cur->next = NULL;
            pcb_head = p;
            while (p->next != NULL) {
                p = p->next;
            }
            p->next = pcb_cur;
        }
    }
    asm volatile("nop");
    pcb_cur = NULL;
    p = pcb_head;
    while (p != NULL) {
        if (p->state == RUNNABLE) {
            p->state = RUNNING;
            pcb_cur = p;
            break;
        }
        p = p->next;
    }
	asm volatile("nop");
    if (pcb_cur == NULL) { 
        
        IDLE();
    } else {
        asm volatile("nop");
		putChar(' ');
      
        tss.esp0 = (uint32_t)&(pcb_cur->stack[KERNEL_STACK_SIZE]);
        tss.ss0  = KSEL(SEG_KDATA);

        gdt[SEG_UCODE] = SEG(STA_X | STA_R, pcb_number(pcb_cur) * PROCESS_MEMORY_SIZE, 0xffffffff, DPL_USER);
        gdt[SEG_UDATA] = SEG(STA_W,         pcb_number(pcb_cur) * PROCESS_MEMORY_SIZE, 0xffffffff, DPL_USER);
        asm volatile("pushl %eax"); 
        asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
        asm volatile("movw %ax, %ds");
		//asm volatile("addl %eax, %esp");
		//asm volatile("ljmp ");
        asm volatile("movw %ax, %es");
        asm volatile("popl %eax");

      	/*
		asm volatile("ldgt va_to(gdtdesc");
		asm volatile("orl $0x1, %eax");
		asm volatile("movw %ax, %ds");
		asm volatile("movw %ax, %es");
		asm volatile("movw %ax, %ss");
		asm volatile("movw %ax, %gs");
		*/
        asm volatile("movl %0, %%esp" ::"r"(&pcb_cur->tf));
		//asm volatile("movw %ax, %fs");
        asm volatile("popl %gs");
        asm volatile("popl %fs");
		//asm volatile("movw %ax, %ds");
        asm volatile("popl %es");
        asm volatile("popl %ds");
        asm volatile("popal");  
        asm volatile("addl $4, %esp");
        asm volatile("addl $4, %esp");
		//asm volatile("addl %eax, %esp");
        //asm volatile("ljmp ..");
        asm volatile("iret");
    }
}

void addPCB(struct ProcessBlock *new_pcb) {
    if (pcb_head == NULL) {
        pcb_head = new_pcb;
    } else {
        struct ProcessBlock * p = pcb_head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = new_pcb;
    }
}

struct ProcessBlock * getPCB() {
	asm volatile("nop");
    struct ProcessBlock * p = pcb_free;
    pcb_free = pcb_free->next;
    p->next = NULL;
    p->sleepTime = 0;
    p->pid = PID_OFFSET + pcb_number(p);
    p->state = RUNNABLE;
    p->timeCount = TIME_UNIT;
    addPCB(p);
    return p;
}

void initPCB() {
    for (int i = 0; i < MAX_PCB - 1; i++) {
        pcb[i].next = &pcb[i + 1];
    }
    pcb[MAX_PCB - 1].next = NULL;
    pcb_free = &pcb[0];
    pcb_head = NULL;
    pcb_cur = NULL;
}

void enterIDLE(uint32_t entry) {
    struct ProcessBlock * p = getPCB();

    pcb_cur = p;
    asm volatile("movl %0, %%eax" ::"r"(USEL(SEG_UDATA)));
    asm volatile("movw %ax, %ds");
    asm volatile("movw %ax, %es");
    asm volatile("movw %ax, %fs");

    p->tf.ss = USEL(SEG_UDATA);
    p->tf.esp = USER_ENTRY + PROCESS_MEMORY_SIZE;
    asm volatile("sti");
    asm volatile("pushfl");  
    asm volatile("cli");

    asm volatile("movl (%%esp), %0" : "=r"(p->tf.eflags) :);

    p->tf.cs = USEL(SEG_UCODE);
    p->tf.eip = entry;
	//TO DO
    p->state = RUNNING;
    p->timeCount = TIME_UNIT;
    p->sleepTime = 0;
    p->pid = PID_OFFSET;

	//...
    asm volatile("movl %0, %%esp" ::"r"(&p->tf.eip));
    asm volatile("iret");
}
