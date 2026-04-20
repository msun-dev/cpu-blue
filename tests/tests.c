#include "../include/cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

uint16_t program0[4] = {
	0xF000,
	0xF003,
	0xF000,
	0xF005,
};

int main(void) {
	cleanRam();
	memmove(RAM, program0, (RAM_LENGTH * sizeof(uint16_t)));

	for (;;) {
		emulateCycle();
		dumpRegisters();
	}

	exit(0);
}
