#include "x86.h"
#include "device.h"

#define INTERRUPT_GATE_32   0xE // 1110
#define TRAP_GATE_32        0xF // 1111

/* IDT表的内容 */
struct GateDescriptor idt[NR_IRQ]; // NR_IRQ=256, defined in x86/cpu.h

/* 初始化一个中断门(interrupt gate) */
static void setIntr(struct GateDescriptor *ptr, uint32_t selector, uint32_t offset, uint32_t dpl) {
	ptr->offset_15_0 = offset & 0xFFFF;
	ptr->segment = selector << 3;
	ptr->pad0 = 0;
	ptr->type = INTERRUPT_GATE_32;
	ptr->system = FALSE;
	ptr->privilege_level = dpl;
	ptr->present = TRUE;
	ptr->offset_31_16 = (offset >> 16) & 0xFFFF;
}

/* 初始化一个陷阱门(trap gate) */
static void setTrap(struct GateDescriptor *ptr, uint32_t selector, uint32_t offset, uint32_t dpl) {
	ptr->offset_15_0 = offset & 0xFFFF;
	ptr->segment = selector << 3;
	ptr->pad0 = 0;
	ptr->type = TRAP_GATE_32;
	ptr->system = FALSE;
	ptr->privilege_level = dpl;
	ptr->present = TRUE;
	ptr->offset_31_16 = (offset >> 16) & 0xFFFF;
}

/* 声明函数，这些函数在汇编代码里定义 */
void irqEmpty();
void irqGProtectFault();
void irqTimerInterrupt();
void irqSyscall();

void initIdt() {
	int i;
	/* 为了防止系统异常终止，所有irq都有处理函数(irqEmpty)。 */
	for (i = 0; i < NR_IRQ; i ++) {
		setTrap(idt + i, SEG_KCODE, (uint32_t)irqEmpty, DPL_KERN);
	}
	/*
	 * init your idt here
	 * 初始化 IDT 表, 为中断设置中断处理函数
	 */

	setTrap(idt + 0xd, SEG_KCODE, (uint32_t)irqGProtectFault, DPL_KERN);

	setIntr(idt + 0x20, SEG_KCODE, (uint32_t)irqTimerInterrupt, DPL_KERN); 

	setTrap(idt + 0x80, SEG_KCODE, (uint32_t)irqSyscall, DPL_USER); 

	/* 写入IDT */
	saveIdt(idt, sizeof(idt));
}
