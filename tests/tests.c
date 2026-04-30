#include "../include/cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define VERBOSE 1

#define SIZE(x) sizeof(x) / sizeof(uint16_t)

uint8_t test_program(uint16_t p[], uint16_t ps, uint32_t cycles_to_emul,
                      uint16_t expected_REG_A);
void printRegs(BlueCpu_t*);
void printRam(BlueCpu_t*);

int test_ct = 0;
int test_cf = 0;

uint16_t test_ADD[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0x1005, //0 | ADD 005 | 2     | A = 0001
	0x1001, //1 | ADD 001 | 4     | A = 0001 + 1001 = 1002
	0x1001, //2 | ADD 001 | 6     | A = 1002 + 1001 = 2003
	0x1000, //3 | ADD 000 | 8     | A = 2003 + 1005 = 3008
	0x1000, //4 | ADD 000 | 10    | A = 3008 + 1005 = >400D<
	0x0001, //5 |  data   |       |
};

uint16_t test_JMP[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xF000, //0 | NOP xxx | 1     | -0x7000 as data
	0xF000, //1 | NOP xxx | 2     |
	0xF005, //2 | NOP xxx | 3     |
	0xA005, //3 | JMP 005 | 4     |
	0x0000, //4 | HLT xxx |       |
	0x1000, //5 | ADD 000 | 6     | A = >F000<
};

uint16_t test_MATH[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xF000, //0 | NOP xxx | 1     |
	0x1004, //1 | ADD 004 | 3     | A = 7FFF
	0x1005, //2 | ADD 005 | 5     | A = 7FFF + FFFF = 0x7FFE
	0x0000, //3 | HLT xxx | 6     |
	0x7FFF, //4 |  data   |       |
	0xFFFF, //5 |  data   |       | 0xFFFF = -1
};

uint16_t test_STALDA[8] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xF000, //0 | NOP xxx | 1     |
	0x6003, //1 | LDA 003 | 3     | A = 0x6D4D
	0xA004, //2 | JMP 004 | 4     |
	0x6D4D, //3 |  data   |       |
	0x7006, //4 | STA 006 | 6     |
	0xA007, //5 | JMP 007 | 7     |
	0x0000, //6 |  data   |       | = >0x6D4D after 4<
	0x0000, //7 | HLT xxx | 8     |
};

int main(void) {
	test_program(test_ADD, SIZE(test_ADD), 10, 0x400D);
	test_program(test_JMP, SIZE(test_JMP), 6, 0xF000);
	test_program(test_MATH, SIZE(test_MATH), 6, 0x7FFE);
	test_program(test_STALDA, SIZE(test_STALDA),  8, 0x6D4D);

	printf("+---\n- Tests executed: %d\n- Tests failed: %d\n",
	       test_ct, test_cf);
	if (test_cf == 0)
		printf("- All tests passed!\n");
	printf("+---\n");

	return (test_cf == 0) ? 0: test_cf;
}

uint8_t test_program(uint16_t p[], uint16_t ps,
                     uint32_t cycles_to_emul, uint16_t expected_REG_A) {
	printf("\nTest #%d:\n", 1 + test_ct++);

	// Initialising CPU. Don't forgt to provide malloc and free functions
	// In this case they are used from stdlib
	BlueCpu_t* cpu = initCpu(malloc, free);
	if (!cpu) {
		printf("Malloc fail on CPU init.\n");
		return 1;
	}

	// Way of loading the program. Only one for the moment.
	// First define uint16_t array of opcodes
	// After that use loadProgram(which cpu, starting address
	//                            program pointer, program size)
	// Care with the size beccause inner memcpy will yoink data after the array
	if (loadProgram(cpu, 0x0000, p, ps)) {
		printf("Size and address exceedes memory! Nothing written\n");
		return 2;
	}

	// Enabling CPU with enableCpu
	enableCpu(cpu);
	if (VERBOSE) {
		printf(" - Post-initiation data: - \n");
		printRegs(cpu);
		printRam(cpu);
	}
	
	// Emulating cycle by calling emulateCycle
	for (uint32_t i = 0; i < cycles_to_emul; i++) {
		if (emulateCycle(cpu) == 1) {
			break;
		}
		if (VERBOSE) {
			printf(" - Post-cycle data: - \n");
			printRegs(cpu);
			printRam(cpu);
		}
	}

	// Comparing register A for testing purposes. Error prone!
	if (getRegister(cpu, REG_A) != expected_REG_A) {
		printf("REG_A has value 0x%04X, was expecting 0x%04X\n",
		       getRegister(cpu, REG_A), expected_REG_A);
		test_cf++;
		return 3;
	}

	printf("Test %d passed! Deinitialising cpu.\n", test_ct);
	deinitCpu(cpu, free);

	return 0;
}

void printRegs(BlueCpu_t* cpu) {
	printf("Registers: \n");
	printf("  PC|  IR| MAR| MBR| DIL| DOL| DSL|   A|  SR|   Z|\n");
	for (uint8_t i = 0; i < REGS_LEN; i++) {
		printf("%04X|", (i == REG_DSL || i == REG_DIL || i == REG_DOL)
		                ? getRegister(cpu, i) & 0x00FF
		                : getRegister(cpu, i)
		);
	}
	putchar('\n');
}

void printRam(BlueCpu_t* cpu) {
	printf("Non-zero memory: \n");
	uint16_t ram_data = 0x0000;
	for (uint32_t i = 0; i < RAM_LEN; i++) {
		ram_data = cpu->ram[i];
		if (ram_data != 0x0000)
			printf("%d - 0x%04X\n", i, ram_data);
	}
}
