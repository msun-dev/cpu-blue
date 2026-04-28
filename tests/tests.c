#include "../include/cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define VERBOSE 1

#define exit(x) printf("Exiting with the code %d\n", x); exit(x)
#define ever(x) ;;
#define lever(x) uint32_t i = 0; i < x; i++
#define fever(x) ;false;

uint8_t test_program(uint16_t p[], uint16_t ps, uint32_t cycles_to_emul,
                      uint16_t expected_REG_A);
void printRegs(BlueCpu_t*);
void printRam(BlueCpu_t*);

int test_ct = 0;
int test_cf = 0;

uint16_t test_data[6] = {
	0xF000,
	0xF003,
	0xF000,
	0xF003,
	0x0000,
	0x1001,
};

uint16_t test_JMP[5] = {
	0xF000,
	0xF003,
	0xF000,
	0xF005,
	0xA000,
};

uint16_t test_LDA[2] = {
	0x6001, //0 LDA 001
	0x000B, //1 data
};

uint16_t test_STA[5] = {
	0x6001, //0 LDA 001
	0x000F, //1 data
	0x7003, //2 STA 003
	0x0000, //3 data
	0x0000, //4 LDA 003
};

uint16_t test_MATH[4] = {
	0x6003, //0 LDA 003
	0x1002, //1 ADD 002
	0x7004, //2 STA 004
	0x0001, //3
};

uint16_t test_ADD[6] = {
	0x1005, //0 | ADD 005 | A = 0001
	0x1001, //1 | ADD 001 | A = 0001 + 1001 = 1002
	0x1001, //2 | ADD 001 | A = 1002 + 1001 = 2003
	0x1000, //3 | ADD 000 | A = 2003 + 1005 = 3008
	0x1000, //4 | ADD 000 | A = 3008 + 1005 = 400D
	0x0001, //5 |  data   |
};

int main(void) {
	//test_program(test_data, 6, 10, 0x0000);
	//test_program(test_JMP, 5, 10, 0x0000));
	//test_program(test_LDA, 4, 5, 0x000B);
	//test_program(test_STA, 5, 100, 0x0000));
	//test_program(test_MATH, 4, 100, 0x0003));
	test_program(test_ADD, 6, 100, 0x400D);

	printf("+---\n- Tests executed: %d\n- Tests failed: %d\n+---\n",
	       test_ct, test_cf);

	return (test_cf == 0) ? 0: test_cf;
}

uint8_t test_program(uint16_t p[], uint16_t ps,
                      uint32_t cycles_to_emul, uint16_t expected_REG_A) {
	printf("\nTest #%d:\n", test_ct++);

	BlueCpu_t* cpu = initCpu(malloc, free);
	if (!cpu) {
		printf("Malloc fail on CPU init.\n");
		return 1;
	}

	if (loadProgram(cpu, 0x0000, p, ps * sizeof(uint16_t))) {
		printf("Size and address exceedes memory! Nothing written\n");
		return 2;
	}

	enableCpu(cpu);
		if (VERBOSE) {
			printf(" - Post-initiation data: - \n");
			printRegs(cpu);
			printRam(cpu);
		}
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

	if (getRegister(cpu, REG_A) != expected_REG_A) {
		printf("REG_A has value 0x%04X, was expecting 0x%04X\n",
		       getRegister(cpu, REG_A), expected_REG_A);
		test_cf++;
		return 3;
	}

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
