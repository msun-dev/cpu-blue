#include "../include/cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ever(x) ;;
#define lever(x) uint32_t i = 0; i < x; i++
#define fever(x) ;false;

uint16_t programm0[4] = {
	0xF000,
	0xF003,
	0xF000,
	0xF005,
};

int main(void) {
	BlueCpu_t* cpu = initCpu();
	if (!cpu) {
		printf("Malloc fail!\n");
		exit(1);
	}
	printregs(cpu);
	loadProgramm(cpu, programm0, 4 * sizeof(uint16_t));
	printf("Memory:\n"); dumpMemory(cpu);

	for (lever(5)) {
		emulateCycle(cpu);
		printregs(cpu);
	}

	exit(0);
}

void printregs(BlueCpu_t* cpu) {
	printf("  PC|  IR| MAR| MBR| DIL| DOL| DSL|   A|  SR|   Z|\n");
	dumpRegisters(cpu);
	printf("-------------------------------------------------|\n");
}
