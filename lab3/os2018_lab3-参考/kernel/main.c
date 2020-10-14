#include "common.h"
#include "device.h"
#include "x86.h"

void kEntry(void) {
    initSerial();  // initialize serial port
    initIdt();     // initialize idt
    initIntr();    // iniialize 8259a
    initTimer();   // iniialize 8253 timer
    initSeg();     // initialize gdt, tss
    initPCB();    // initialize process table
    init_vga();    // clear screen
    loadUMain();   // load user program, enter user space

    while (1);
}
