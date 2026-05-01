#include "../include/cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define VERBOSE 1

#define SIZE(x) sizeof(x) / sizeof(uint16_t)

uint8_t testProgram(uint16_t p[], uint16_t ps, uint32_t cycles_to_emul,
                      uint16_t expected_REG_A);
void printRegs(BlueCpu_t*);
void printRam(BlueCpu_t*);

int test_ct = 0; // test  counter
int test_cf = 0; // tests failed

uint16_t test_ADD[8] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0x1006, //0 | ADD 005 | 2     | A = 0005
	0x1006, //1 | ADD 001 | 4     |
	0x1006, //2 | ADD 001 | 6     |
	0x1006, //3 | ADD 000 | 8     | ...
	0x1006, //4 | ADD 000 | 10    |
	0x1006, //5 | ADD 000 | 12    | A = >001E<
	0x0005, //6 |  data   | 13    |
	0x0000, //7 | HLT     | 14    |
};

uint16_t test_XOR[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA003, //0 | JMP 002 | 1     |
	0x888A, //1 |  data   |       |
	0xFD71, //2 |  data   |       |
	0x1001, //3 | ADD 001 | 3     | A = 888A
	0x2002, //4 | XOR 002 | 5     | A = 888A ^ FD71 = 75FB
	0x0000, //5 | HLT     | 6     |
};

uint16_t test_AND[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA003, //0 | JMP 002 | 1     |
	0x888A, //1 |  data   |       |
	0xFD71, //2 |  data   |       |
	0x1001, //3 | ADD 001 | 3     | A = 888A
	0x3002, //4 | AND 002 | 5     | A = 888A & FD71 = 8800
	0x0000, //5 | HLT     | 6     |
};

uint16_t test_IOR[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA003, //0 | JMP 002 | 1     |
	0x888A, //1 |  data   |       |
	0xFD71, //2 |  data   |       |
	0x1001, //3 | ADD 001 | 3     | A = 888A
	0x4002, //4 | IOR 002 | 5     | A = 888A ^ FD71 = FDFB
	0x0000, //5 | HLT     | 6     |
};

uint16_t test_NOT[6] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA003, //0 | JMP 002 | 1     |
	0x888A, //1 |  data   |       |
	0xFD71, //2 |  data   |       |
	0x1001, //3 | ADD 001 | 3     | A = 888A
	0x5002, //4 | NOT xxx | 5     | A = !888A = 0x7775
	0x0000, //5 | HLT     | 6     |
};

uint16_t test_JMP[7] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xF000, //0 | NOP xxx | 1     |
	0xF000, //1 | NOP xxx | 2     |
	0xF005, //2 | NOP xxx | 3     |
	0xA005, //3 | JMP 005 | 4     |
	0x0000, //4 | HLT xxx |       |
	0x1000, //5 | ADD 000 | 6     | A = >F000<
	0x0000, //6 | HTL xxx | 7     |
};

uint16_t test_SRJ[27] = {
// CODE   addr  ASM OP   Cycles  Registers state
  /* Start */
	0x8007, //0 | SRJ 004 | 1     | Running subroutine on 007
	0xA017, //1 | JMP 00x | 20    | Going to other routine
  /* Storage */
	0x0000, //2 |  data   |       | SRJ pointer, 0001 after jump and STA
	0x1111, //3 |  data   |       | const for sub-routine
	0x5555, //4 |  data   |       | const for post sub-routine
	0x0000, //5 |  data   |       | data storage for sub routine (3333)
	0x0000, //6 |  data   |       | data storage for post sub-routine
  /* Sub-Routine */
	0x7002, //7 | STA 002 | 3     | ??? [002] = 0001
	0x6003, //8 | LDA 003 | 5     | A = 1111
	0x1003, //9 | ADD 003 | 7     | A = 1111 + 1111 = 2222
	0x1003, //A | ADD 003 | 9     | A = 2222 + 1111 = 3333
	0x7005, //B | STA 004 | 11    | [005] = 3333
	0xA00F, //C | JMP 00F | 12    | Reconstructing pointer
	0x0000, //D | HLT xxx |       |
	0x0000, //E | HLT xxx |       |
	/* Reconstructor */
	0x6002, //F | LDA 002 | 13    |
	0xA013, //10| JMP 013 | 14    | jump over consts
	0xA000, //11|  data   |       | const OP_JMP
	0x0000, //12|  data   |       | zero const
	0x4011, //13| IOR 011 | 15    | Combining loaded adress with OP_JMP
	0x7015, //14| STA 015 | 17    | Storing it in jmp-bck
	0x0000, //15| jmp-bck | 19    | jumps back to non-sub-routine. Should be A001
	0x0000, //16| HLT xxx |       | trap just in case.
	/* Not Sub-Routine */
	0x6005, //17| LDA 005 | 22    | A = 3333
	0x1004, //1A| ADD 004 | 24    | A = 3333 + 5555 = 8888
	0x0000, //1B| HLT xxx | 25    |
};

uint16_t test_JMA[11] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA004, //0 | JMP 003 | 1     |
	0x7123, //1 |  data   |       |
	0x8000, //2 |  data   |       |
	0x6DAD, //3 |  data   |       | loaded to A on success
	0x1001, //4 | ADD 001 | 3     | A = 7123 dec; 01110001 00100011 bin
	0x9008, //5 | JMA 008 | 4     | should do nothing
	0x4002, //6 | IOR 002 | 6     | A = 7123 | 8001 = 11110001 00100011
	0x9009, //7 | JMA 009 | 7     | should jump to 009
	0x0000, //8 | HLT xxx |       | trap for JMA
	0x6003, //9 | LDA 003 | 9     | A = >6DAD<
	0x0000, //10| HTL xxx | 10    |
};

uint16_t test_STALDA[9] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA004, //0 | JMP 003 | 1     |
	0x6D4D, //1 |  data   |       | data for LDA
	0x0000, //2 |  data   |       | data for STA
	0x0001, //3 |  data   |       | bit
	0x6001, //4 | LDA 001 | 3     | A = 6D4D
	0x1003, //5 | ADD 003 | 5     | A = 6D4D + 0001 = 6D4E
	0x7002, //6 | STA 002 | 7     | ram[002] = 6D4E
	0x6002, //7 | LDA 002 | 9     | A = 6D4E
	0x0000, //8 | NOP xxx | 10    |
};

uint16_t test_RAL[5] = {
// CODE   addr  ASM OP   Cycles  Registers state
	0xA002, //0 | JMP 002 | 1     |
	0xAAAA, //1 |         |       |
	0x6001, //2 | LDA 001 | 3     | A = AAAA = 10101010 10101010
	0xD000, //3 | RAL xxx | 4     | A = 5555 = 01010101 01010101
	0x0000, //4 | HLT xxx | 5     |
};

//Overflow

int main(void) {
	testProgram(test_ADD,    SIZE(test_ADD),    15, 0x001E);
	testProgram(test_XOR,    SIZE(test_XOR),     7, 0x75FB);
	testProgram(test_AND,    SIZE(test_AND),     7, 0x8800);
	testProgram(test_IOR,    SIZE(test_IOR),     7, 0xFDFB);
	testProgram(test_NOT,    SIZE(test_NOT),     6, 0x7775);
	testProgram(test_JMP,    SIZE(test_JMP),     7, 0xF000);
	testProgram(test_SRJ,    SIZE(test_SRJ),    26, 0x8888);
	testProgram(test_JMA,    SIZE(test_JMA),    10, 0x6DAD);
	testProgram(test_STALDA, SIZE(test_STALDA), 11, 0x6D4E);
	testProgram(test_RAL,    SIZE(test_RAL),    10, 0x5555);

	printf("+---\n- Tests executed: %d\n- Tests failed: %d\n",
	       test_ct, test_cf);
	if (test_cf == 0)
		printf("- All tests passed!\n");
	printf("+---\n");

	return (test_cf == 0) ? 0: test_cf;
}

uint8_t testProgram(uint16_t p[], uint16_t ps,
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
			printf("HLT command\n");
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
			printf("0x%04X - 0x%04X\n", i, ram_data);
	}
}
