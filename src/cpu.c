#include "../include/cpu.h"

#define FETCH_CYCLE_END(x, y, z) \
	if (tick == 6) {      \
		x;                  \
	}                     \
	else if (tick == 7) { \
		y;                  \
	}                     \
	else if (tick == 8) { \
		z;                  \
	}                     \

// Ready
BlueCpu_t* initCpu() {
	BlueCpu_t* cpu = NULL;
	cpu = malloc(sizeof(BlueCpu_t));
	if (!cpu) {
		free(cpu);
		return NULL;
	}
	cpu->clock_pulse = 0;
	cpu->state = FETCH;
	clearRam(cpu);
	clearRegisters(cpu);
	return cpu;
}

void clearRam(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < RAM_LENGTH; cpu->ram[i++] = 0x0000);
}

void clearRegisters(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < REGS_LENGTH; setRegister(cpu, i++, 0x0000));
}

void loadProgramm(BlueCpu_t* cpu, uint16_t* programm, uint32_t size) {
	clearRam(cpu);
	memcpy(cpu->ram, programm, size);
}

// Process
void emulateCycle(BlueCpu_t* cpu) {
	uint8_t* clock_pulse = &(cpu->clock_pulse);
	for (*clock_pulse = 1; *clock_pulse < PULSE_AMT + 1; *clock_pulse += 1)
		processTick(cpu, *clock_pulse);
}

void processTick(BlueCpu_t* cpu, uint8_t tick) {
	switch (tick) {
	case 1:
		break;
	case 2:
		if (cpu->state == FETCH)
			incRegister(cpu, REG_PC);
		break;
	case 3:
		if (cpu->state == FETCH)
			setRegister(cpu, REG_MBR, 0x0000);
		break;
	case 4:
		if (cpu->state == FETCH) {
			setRegister(cpu, REG_IR, 0x0000);
			setRegister(cpu, REG_MBR, cpu->ram[getRegister(cpu, REG_MAR)]);
		}
		break;
	case 5:
		if (cpu->state == FETCH)
			setRegister(cpu, REG_IR, getRegister(cpu, REG_MBR));
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		break;
	default:
		break;
	}
	execInstruction(cpu, getInstruction(cpu), tick);
}

// Registers
void setRegister(BlueCpu_t* cpu, Register reg, uint16_t value) {
	cpu->registers[reg] = value;
}

uint16_t getRegister(BlueCpu_t* cpu, Register reg) {
	return cpu->registers[reg];
}

void incRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, getRegister(cpu, reg) + 1);
}

// Instructions
uint8_t getInstruction(BlueCpu_t* cpu) {
	return ((getRegister(cpu, REG_IR) & 0xF000) >> 12);
}

void execInstruction(BlueCpu_t* cpu, Instruction instr, uint8_t tick) {
	switch(instr) {
	case OP_HLT:
		break;
	case OP_ADD:
		break;
	case OP_XOR:
		break;
	case OP_AND:
		break;
	case OP_IOR:
		break;
	case OP_NOT:
		break;
	case OP_LDA:
		break;
	case OP_STA:
		break;
	case OP_SRJ:
		FETCH_CYCLE_END(
			if (getRegister(cpu, REG_A) == 1) // pulse == 6
				setRegister(cpu, REG_PC, 0),
			if (getRegister(cpu, REG_A) == 1) // pulse == 7
				setRegister(cpu, REG_PC, getRegister(cpu, REG_IR) & 0x0FFF),
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC)) // pulse == 8
		);
		break;
	case OP_JMA:
		FETCH_CYCLE_END(
			if (getRegister(cpu, REG_A) == 1) // pulse == 6
				setRegister(cpu, REG_PC, 0),
			if (getRegister(cpu, REG_A) == 1) // pulse == 7
				setRegister(cpu, REG_PC, getRegister(cpu, REG_IR) & 0x0FFF),
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC)); // pulse == 8
		);
		break;
	case OP_JMP:
		FETCH_CYCLE_END(
			setRegister(cpu, REG_PC, 0), // pulse == 6
			setRegister(cpu, REG_PC, (getRegister(cpu, REG_IR) & 0x0FFF)), // pulse == 7
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC)) // pulse = 8
		);
		break;
	case OP_INP:
		break;
	case OP_OUT:
		break;
	case OP_RAL:
		break;
	case OP_CSA:
		FETCH_CYCLE_END(
			setRegister(cpu, REG_A, 0x0000), // pulse = 6
			setRegister(cpu, REG_A, getRegister(cpu, REG_SR)), // pulse = 7
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC)) // pulse = 8
		);
		break;
	case OP_NOP:
		if (tick == 8) {
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
		}
		break;
	default:
		break;
	}
}

// Debug
void dumpRegisters(BlueCpu_t* cpu) {
	for (uint16_t i = 0; i < REGS_LENGTH; i++)
		printf("%04X|", (i == REG_DSL || i == REG_DIL || i == REG_DOL)
		                 ? getRegister(cpu, i) & 0x00FF
		                 : getRegister(cpu, i)
		);
	putchar('\n');
}

void dumpMemory(BlueCpu_t* cpu) {
	uint16_t ram_data = 0x0000;
	for (uint32_t i = 0; i < RAM_LENGTH; i++) {
		ram_data = cpu->ram[i];
		if (ram_data != 0x0000)
			printf("%d - 0x%4X\n", i, ram_data);
	}
}

