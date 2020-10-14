#include "device.h"
#include "x86.h"

/* defined in <sys/syscall.h> */
#define EXIT 1
#define FORK 2
#define PRINT 4
#define SLEEP 5  // user defined

void scheduler();

void syscallHandle(struct TrapFrame *tf);

void timerInterruptHandle(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) {
    /*
     * 中断处理程序
     */
	//putChar('a');
    /* Reassign segment register */
    asm volatile("movl %0, %%eax" ::"r"(KSEL(SEG_KDATA)));
    asm volatile("movw %ax, %ds");
	asm volatile("nop");
    asm volatile("movw %ax, %fs");
    asm volatile("movw %ax, %es");
    asm volatile("movl %0, %%eax" ::"r"(KSEL(7)));
	asm volatile("nop");
    asm volatile("movw %ax, %gs");
	asm volatile("nop");
    switch (tf->irq) {
        case -1:
            break;
        case 0xd:
            GProtectFaultHandle(tf);
            break;
        case 0x20:
            timerInterruptHandle(tf);
            break;
        case 0x80:
            syscallHandle(tf);
            break;
        default:
            assert(0);
    }
}

void sys_Exit(struct TrapFrame *tf) {
    
    struct ProcessBlock *p, *q;
    uint32_t t_pid = pcb_cur->pid;
	//putChar('2');
    asm volatile("nop");
    if (pcb_head->pid == t_pid) {
		//putChar('3');
        p = pcb_head;
        pcb_head = pcb_head->next;
        p->next = pcb_free;
        pcb_free = p;
		asm volatile("nop");
    } else {
        p = pcb_head, q = pcb_head->next;
        while (q != NULL) {
			//putChar('2');
            if (q->pid == t_pid) {
                p->next = q->next;
                q->next = pcb_free;
                pcb_free = q;
                break;
            }
            p = q;
            q = q->next;
        }
    }

    pcb_cur = NULL;
    scheduler();
}

void sys_Fork(struct TrapFrame *tf) {
    
    struct ProcessBlock *p = getPCB();

 	/*
	PDE* kpdir = get_kpdir();
	memset(updir, 0, NR_PDE*sizeof(PDE));
	memcpy(&updir[KOFFSET / PT_SIZE);
	*/   
    int father_process_address = USER_ENTRY + pcb_number(pcb_cur) * PROCESS_MEMORY_SIZE;
    int child_process_address = USER_ENTRY + pcb_number(p) * PROCESS_MEMORY_SIZE;
    for (int i = 0; i < PROCESS_MEMORY_SIZE; i++) {
        *((uint8_t *)child_process_address + i) = *((uint8_t *)father_process_address + i);
    }
	asm volatile("nop");
    for (int i = 0; i < KERNEL_STACK_SIZE; i++) {
        p->stack[i] = pcb_cur->stack[i];
    }
	putChar('f');
	putChar(' ');
    p->tf.eax = 0;             // child process return value
    pcb_cur->tf.eax = p->pid;  // father process return value
	putChar('c');
    pcb_cur->state = RUNNABLE;

    scheduler();
}

void sys_Sleep(struct TrapFrame *tf) {
    //used for process to block itself
	//transfer running to blocked,reset sleeptime,and scheduler()
    asm volatile("nop");
    pcb_cur->sleepTime = tf->ebx;
    pcb_cur->state = BLOCKED;
	putChar('^');
    scheduler();
}

void sys_Print(struct TrapFrame *tf) {
    asm volatile("nop");
    static int row = 0, col = 0;
    char c = '\0';

    // offset
    tf->ecx += ((pcb_cur - pcb) * PROCESS_MEMORY_SIZE);

	asm volatile("nop");
    if (tf->ebx == 1 || tf->ebx == 2) {  
        int i;
        for (i = 0; i < tf->edx; i++) {
            c = *(char *)(tf->ecx + i);
            asm volatile("nop");
            if (c == '\n') {
                row++;
                col = 0;
                continue;
            }
            if (col == 80) {
                row++;
                col = 0;
            }
            video_print(row, col++, c);
        }
        tf->eax = tf->edx;  // return value
    }
}

void syscallHandle(struct TrapFrame *tf) {
    /* 实现系统调用*/
	asm volatile("nop");
    switch (tf->eax) {
        case EXIT:
            sys_Exit(tf);
            break;
        case FORK:
            sys_Fork(tf);
            break;
        case PRINT:
            sys_Print(tf);
            break;
        case SLEEP:
            sys_Sleep(tf);
            break;
   
        default:
            assert(0);
    }
}

void timerInterruptHandle(struct TrapFrame *tf) {
    asm volatile("nop");
    struct ProcessBlock *p = pcb_head;
    while (p != NULL) {
        if (p->sleepTime > 0) {
            --(p->sleepTime);
            if (p->sleepTime == 0) {
                p->state = RUNNABLE;
            }
        }
        p = p->next;
    }
//	putChar('a');
	/*
		putChar('b');
		asm volatile("");
		memcpy(pcb_cur->timeCount..
		//..
	*/
    if (pcb_cur == NULL) { 
        scheduler();
        return;
    }
	asm volatile("nop");
    (pcb_cur->timeCount)--;
    if (pcb_cur->timeCount == 0) {
        pcb_cur->state = RUNNABLE;
        pcb_cur->timeCount = TIME_UNIT;
        
        scheduler();
    }
    return;
}

void GProtectFaultHandle(struct TrapFrame *tf) {
    assert(0);
    return;
}
