#include "x86.h"
#include "device.h"
extern SegDesc gdt[NR_SEGMENTS];
extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

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
	/*XXX Save esp to stackTop */
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
		case 0x80:
			syscallHandle(sf);
			break;
		default:assert(0);
	}
	/*XXX Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf) {
	assert(0);
	return;
}
void my_print(char c[]){
		for(int i=0;c[i]!='\0';i++)
			putChar(c[i]);

}
void timerHandle(struct StackFrame *sf) {
	//putChar(current+'0');
	int i;
	uint32_t tmpStackTop;
	i = (current+1) % MAX_PCB_NUM;
    // make blocked processes sleep time -1, sleep time to 0, re-run
		for(int j=i;j!=current;j=(j+1)%MAX_PCB_NUM){
			if(pcb[j].state==STATE_BLOCKED){
				if(pcb[j].sleepTime!=0)
				pcb[j].sleepTime--;
				else pcb[j].state=STATE_RUNNABLE;
			}
		}
		
    // time count not max, process continue
		if(pcb[current].timeCount!=0){
			pcb[current].timeCount--;
			//putChar(current+'0');
			//putChar(' ');
			return ;

		}
		
    // else switch to another process
		else {
			pcb[current].state=STATE_RUNNABLE;
			pcb[current].timeCount=MAX_TIME_COUNT;
			for(int j=i;j!=current;j=(j+1)%MAX_PCB_NUM){
				if(pcb[j].state==STATE_RUNNABLE){
					pcb[j].state=STATE_RUNNING;
					pcb[j].timeCount=MAX_TIME_COUNT;
					current=j;
					break;
				}
			}
		}	 
			
		/* echo pid of selected process */
		uint32_t pid=pcb[current].pid;
		//putChar(pid+'0');
		putChar(pid+'0');
		/*XXX recover stackTop of selected process */
			tmpStackTop=pcb[current].stackTop;
			pcb[current].stackTop=pcb[current].prevStackTop;
			pcb[current].prevStackTop=tmpStackTop;
    // setting tss for user process
			tss.esp0 = pcb[current].stackTop;
     	//tss.ss0  = USEL(2+pcb[current].pid*2);
			asm volatile("movl %0,%%esp"::"m"(tmpStackTop));
		// switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	
}

void syscallHandle(struct StackFrame *sf) {
	switch(sf->eax) { // syscall number
		case 0:
			syscallWrite(sf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(sf);
			break; // for SYS_FORK
		case 2:
			syscallExec(sf);
			break; // for SYS_EXEC
		case 3:
			syscallSleep(sf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(sf);
			break; // for SYS_EXIT
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
	int sel = sf->ds; //TODO segment selector for user data, need further modification
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
		//asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		//asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
	return;
}
void mmcpy(uint8_t* dst,uint8_t* src,size_t size ){
	for(size_t i=0;i<size;i++)
		*(dst+i)=*(src+i);
}

void InitialPcb(ProcessTable* father,ProcessTable* son,int index){
	for(int i=0;i<MAX_STACK_SIZE;i++)
		son->stack[i]=father->stack[i];
	
	son->state=STATE_RUNNABLE;
	son->pid=index;
	son->stackTop=(uint32_t)&son->regs;
	son->prevStackTop=(uint32_t)&son->stackTop;
	son->timeCount=0;
	son->sleepTime=0;
	//son->regs=father->regs;
	mmcpy((uint8_t*)&(son->regs),(uint8_t*)&(father->regs),sizeof(son->regs));
	son->regs.cs=USEL(1+2*index);
	son->regs.ds=USEL(2+2*index);
	son->regs.es=USEL(2+2*index);
	son->regs.fs=USEL(2+2*index);
	son->regs.ss=USEL(2+2*index);
	son->regs.gs=USEL(2+2*index);

}
void syscallFork(struct StackFrame *sf) {
	//my_print("do fork : ");
    // find empty pcb
		//while(1);
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD)
			break;
	}
	if (i != MAX_PCB_NUM) {
		/*XXX copy userspace
		  XXX enable interrupt
		 */
		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (i+1)*0x100000) = *(uint8_t *)(j + (current+1)*0x100000);
			//asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		}
		/*XXX disable interrupt
		 */
		disableInterrupt();
		/*XXX set pcb
		  XXX pcb[i]=pcb[current] doesn't work
		*/
		InitialPcb(&pcb[current],&pcb[i],i);
		/*XXX set regs */
		
		/*XXX set return value */
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
	uint32_t tmpStackTop;
	int i = (current+1) % MAX_PCB_NUM;
	pcb[current].state=STATE_BLOCKED;
	pcb[current].sleepTime=sf->ecx;


	for(int j=i;j!=current;j=(j+1)%MAX_PCB_NUM){
		if(pcb[j].state==STATE_RUNNABLE){
			pcb[j].state=STATE_RUNNING;
			pcb[j].timeCount=MAX_TIME_COUNT;
			current=j;
			break;
		}
	}
		/* echo pid of selected process */
		uint32_t pid=pcb[current].pid;
		//putChar(pid+'0');
		putChar(pid+'0');
		/*XXX recover stackTop of selected process */
			tmpStackTop=pcb[current].stackTop;
			pcb[current].stackTop=pcb[current].prevStackTop;
			pcb[current].prevStackTop=tmpStackTop;
    // setting tss for user process
			tss.esp0 = pcb[current].stackTop;
     	//tss.ss0  = USEL(2+pcb[current].pid*2);
			asm volatile("movl %0,%%esp"::"m"(tmpStackTop));
		// switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	return;
}

void syscallExit(struct StackFrame *sf) {
	uint32_t tmpStackTop;
	int i = (current+1) % MAX_PCB_NUM;
	pcb[current].state=STATE_DEAD;

	for(int j=i;j!=current;j=(j+1)%MAX_PCB_NUM){
		if(pcb[j].state==STATE_RUNNABLE){
			pcb[j].state=STATE_RUNNING;
			pcb[j].timeCount=MAX_TIME_COUNT;
			current=j;
			break;
		}
	}
		/* echo pid of selected process */
		uint32_t pid=pcb[current].pid;
		//putChar(pid+'0');
		putChar(pid+'0');
		/*XXX recover stackTop of selected process */
			tmpStackTop=pcb[current].stackTop;
			pcb[current].stackTop=pcb[current].prevStackTop;
			pcb[current].prevStackTop=tmpStackTop;
    // setting tss for user process
			tss.esp0 = pcb[current].stackTop;
     	//tss.ss0  = USEL(2+pcb[current].pid*2);
			asm volatile("movl %0,%%esp"::"m"(tmpStackTop));
		// switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	return;
}
