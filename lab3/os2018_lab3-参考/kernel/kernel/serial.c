#include "x86.h"
#include "device.h"

void initSerial(void) {
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x80);
	outByte(SERIAL_PORT + 0, 0x01);
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x03);
	outByte(SERIAL_PORT + 2, 0xC7);
	outByte(SERIAL_PORT + 4, 0x0B);
}

static inline int serialIdle(void) {
	return (inByte(SERIAL_PORT + 5) & 0x20) != 0;
}

void putChar(char ch) {
	while (serialIdle() != TRUE);
	outByte(SERIAL_PORT, ch);
}

void update_cursor(int r, int c) {
	uint16_t pos = r * 80 + c;
	outByte(0x3d4, 0x0f);
	outByte(0x3d5, 0xff & pos);
	outByte(0x3d4, 0x0e);
	outByte(0x3d5, pos >> 8);
}


void video_print(int row, int col, char c) {
	asm volatile("nop");
	asm ("movl %0, %%edi;"			: :"r"(((80 * row + col) * 2))  :"%edi");
	asm ("movw %0, %%eax;"			: :"r"(0x0c00 | c) 				:"%eax"); 
	asm ("movw %%ax, %%gs:(%%edi);" : : 							:"%edi"); 
	asm volatile("nop");
	update_cursor(row, col);
}

//clear screen
void init_vga() {
	update_cursor(0, 0);
	for (int i = 0; i < 10 * 80; i ++) {
		*((uint16_t *)0xb8000 + i) = (0x0c << 8);
	}
}
