#include "../include/cpu.h"

// User API
/// Initialisation
BlueCpu_t* initCpu() {
	BlueCpu_t* cpu = NULL;
	cpu = malloc(sizeof(BlueCpu_t));
	if (!cpu) {
		free(cpu);
		return NULL;
	}
	cpu->clock_pulse = 0;
	cpu->state = ST_FETCH;
	clearRam(cpu);
	clearRegisters(cpu);

	cpu->R = false;
	cpu->TRA = false;
	return cpu;
}

void loadProgramm(BlueCpu_t* cpu, uint16_t* programm, uint32_t size) {
	clearRam(cpu);
	memcpy(cpu->ram, programm, size);
}

void deinitCpu(BlueCpu_t* cpu) {
	free(cpu);
}

/// Process
void emulateCycle(BlueCpu_t* cpu) {
	uint8_t* clock_pulse = &(cpu->clock_pulse);
	for (*clock_pulse = 1; *clock_pulse < PULSE_AMT + 1; *clock_pulse += 1)
		processTick(cpu, *clock_pulse);
}

/// Debug
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

// CPU logic
/// Ready
void clearRam(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < RAM_LENGTH; cpu->ram[i++] = 0x0000);
}

void clearRegisters(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < REGS_LENGTH; setRegister(cpu, i++, 0x0000));
}

/// Process
void processTick(BlueCpu_t* cpu, uint8_t tick) {
	switch (tick) {
	case 1:
		break;
	case 2:
		if (cpu->state == ST_FETCH)
			incRegister(cpu, REG_PC);
		break;
	case 3:
		if (cpu->state == ST_FETCH)
			setRegister(cpu, REG_MBR, 0x0000);
		break;
	case 4:
		if (cpu->state == ST_FETCH) {
			setRegister(cpu, REG_IR, 0x0000);
			setRegister(cpu, REG_MBR, cpu->ram[getRegister(cpu, REG_MAR)]);
		}
		break;
	case 5:
		if (cpu->state == ST_FETCH)
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

/// Registers
void setRegister(BlueCpu_t* cpu, Register reg, uint16_t value) {
	cpu->registers[reg] = value;
}

uint16_t getRegister(BlueCpu_t* cpu, Register reg) {
	return cpu->registers[reg];
}

void clrRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, 0x0000);
}
void incRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, getRegister(cpu, reg) + 1);
}

/// Instructions
uint8_t getInstruction(BlueCpu_t* cpu) {
	return ((getRegister(cpu, REG_IR) & 0xF000) >> 12);
}

void execInstruction(BlueCpu_t* cpu, Instruction instr, uint8_t tick) {
	switch (instr) {

	case OP_HLT:
		switch (tick) {
		case 7:
			;//cpu->run = false;
			break;
		case 8:
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
			break;
		}
		break;

	case OP_ADD:
		// Disable cpu when -2^15>sum>2^15-1
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
		switch (tick) {
			case 6:
				setRegister(cpu, REG_A, getRegister(cpu, REG_PC) & 0x0FFF);
				break;
			case 7:
				clrRegister(cpu, REG_PC);
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
				setRegister(cpu, REG_PC,  getRegister(cpu, REG_IR) & 0x0FFF);
				break;
		}
		break;

	case OP_JMA:
		switch (tick) {
			case 6:
				if ((getRegister(cpu, REG_A) & 0x8000)) // NB
					clrRegister(cpu, REG_PC);
				break;
			case 7:
				if ((getRegister(cpu, REG_A) & 0x8000)) // NB
					setRegister(cpu, REG_PC, getRegister(cpu, REG_IR) & 0x0FFF);
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
		}
		break;

	case OP_JMP:
		switch (tick) {
		case 6:
			setRegister(cpu, REG_PC, 0);
			break;
		case 7:
			setRegister(cpu, REG_PC, (getRegister(cpu, REG_IR) & 0x0FFF));
			break;
		case 8:
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
			break;
		}
		break;

	case OP_INP: // Reading INPut from device
		// A (XXYY): XX = 0x00, XX <- xx from YY device.
		break;

	case OP_OUT: // Sending OUTput to a device
		if (cpu->state == ST_FETCH) {
			switch (tick) {
			case 6:
				setRegister(cpu, REG_DOL, getRegister(cpu, REG_A) >> 8 & 0xF000);
				setRegister(cpu, REG_DSL, getRegister(cpu, REG_A) & 0x003F);
				break;
			case 7:
				cpu->TRA = true;
				break;
			case 8:
				cpu->state = ST_EXECUTE;
				break;
			}
		}
		else if (cpu->state == ST_EXECUTE) {
			switch (tick) {
			case 6:
				if (cpu->R == true) {
					cpu->TRA = false;
				}
				break;
			case 8:
				if (cpu->TRA == false) {
					cpu->state = ST_FETCH;
					setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				}
				break;
			}
		}
		break;

	case OP_RAL:
		if (cpu->state == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				cpu->state = ST_EXECUTE;
				break;
			}
		}
		else if (cpu->state == ST_EXECUTE) {
			switch (tick) {
				case 1:
					clrRegister(cpu, REG_A);
					break;
				case 2:
					setRegister(cpu, REG_A, 2 * getRegister(cpu, REG_Z));
					break;
				case 8:
					setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
					cpu->state = ST_FETCH;
					break;
			}
		}
		break;

	case OP_CSA:
		switch (tick) {
			case 6:
				clrRegister(cpu, REG_A);
				break;
			case 7:
				setRegister(cpu, REG_A, getRegister(cpu, REG_SR));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
		}
		break;

	case OP_NOP:
		if (tick == 8) {
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
		}
		break;
	default:
		printf("Something really bad happened! You are in the default case!\n");
		break;
	}
}

