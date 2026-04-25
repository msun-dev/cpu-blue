#include "../include/cpu.h"

// TODO: write leftover ops
// TODO: implement switches
// TODO: implement device interface
// TODO: Remove stdio
// TODO: Replace bool

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
	setState(cpu, ST_FETCH);
	clearRam(cpu);
	clearRegisters(cpu);
	setSwitch(cpu, SW_POWER, false);
	setSwitch(cpu, SW_READY, false);
	setSwitch(cpu, SW_TRA, false);

	return cpu;
}

void loadProgramm(BlueCpu_t* cpu, uint16_t* programm, uint32_t size) {
	clearRam(cpu);
	memcpy(cpu->ram, programm, size);
}

void deinitCpu(BlueCpu_t* cpu) {
	free(cpu);
}

/// Switches
void enableCpu(BlueCpu_t* cpu) {
	setSwitch(cpu, SW_POWER, true);
}

void disableCpu (BlueCpu_t* cpu) {
	setSwitch(cpu, SW_POWER, false);
}

/// Process
uint8_t emulateCycle(BlueCpu_t* cpu) {
	if (getSwitch(cpu, SW_POWER) == false) {
		return 1;
	}

	uint8_t* clock_pulse = &(cpu->clock_pulse);
	for (*clock_pulse = 1; *clock_pulse < PULSE_AMT + 1; *clock_pulse += 1)
		processTick(cpu, *clock_pulse);
	return 0;
}

/// Debug
void dumpRegisters(BlueCpu_t* cpu) {
	for (uint16_t i = 0; i < REGS_LEN; i++)
		printf("%04X|", (i == REG_DSL || i == REG_DIL || i == REG_DOL)
		                ? getRegister(cpu, i) & 0x00FF
		                : getRegister(cpu, i)
		);
	putchar('\n');
}

void dumpMemory(BlueCpu_t* cpu) {
	uint16_t ram_data = 0x0000;
	for (uint32_t i = 0; i < RAM_LEN; i++) {
		ram_data = cpu->ram[i];
		if (ram_data != 0x0000)
			printf("%d - 0x%4X\n", i, ram_data);
	}
}

// CPU logic
/// Ready
void clearRam(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < RAM_LEN; cpu->ram[i++] = 0x0000);
}

void clearRegisters(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < REGS_LEN; setRegister(cpu, i++, 0x0000));
}

/// States
void setState(BlueCpu_t* cpu, State s) {
	cpu->state = s;
}

State getState(BlueCpu_t* cpu) {
	return cpu->state;
}

/// Switches
void setSwitch(BlueCpu_t* cpu, Switch sw, bool value) {
	cpu->status_switches[sw] = value;
}

Switch getSwitch(BlueCpu_t* cpu, Switch sw) {
	return cpu->status_switches[sw];
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

/// Process
void processTick(BlueCpu_t* cpu, uint8_t tick) {
	switch (tick) {
	case 1:
		break;
	case 2:
		if (getState(cpu) == ST_FETCH)
			incRegister(cpu, REG_PC);
		break;
	case 3:
		if (getState(cpu) == ST_FETCH)
			setRegister(cpu, REG_MBR, 0x0000);
		break;
	case 4:
		if (getState(cpu) == ST_FETCH) {
			setRegister(cpu, REG_IR, 0x0000);
			setRegister(cpu, REG_MBR, cpu->ram[getRegister(cpu, REG_MAR)]);
		}
		break;
	case 5:
		if (getState(cpu) == ST_FETCH)
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

/// Instructions
uint8_t getInstruction(BlueCpu_t* cpu) {
	return ((getRegister(cpu, REG_IR) & 0xF000) >> 12);
}

void execInstruction(BlueCpu_t* cpu, Instruction instr, uint8_t tick) {
	switch (instr) {
	case OP_HLT:
		switch (tick) {
		case 7:
			setSwitch(cpu, SW_POWER, false);
			break;
		case 8:
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
			break;
		}
		break;

	case OP_ADD:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			//initial read?
			case 3:
				clrRegister(cpu, REG_A);
				clrRegister(cpu, REG_MBR);
				break;
			case 7:;
				uint32_t result = getRegister(cpu, REG_Z) + getRegister(cpu, REG_MBR);
				if ((getRegister(cpu, REG_Z) & 0x8000)
				    && (getRegister(cpu, REG_MBR) & 0x8000)
				    && !(result & 0x8000))
					setSwitch(cpu, SW_POWER, false);
				else if (!(getRegister(cpu, REG_Z) & 0x8000)
				         && !(getRegister(cpu, REG_MBR) & 0x8000)
				         && (result & 0x8000))
					setSwitch(cpu, SW_POWER, false);
				setRegister(cpu, REG_A, (uint16_t)result);
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
		break;

	case OP_XOR:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			//initial read?
			case 3:
				clrRegister(cpu, REG_A);
				clrRegister(cpu, REG_MBR);
				break;
			case 7:
				setRegister(cpu, REG_A,
				            getRegister(cpu, REG_Z) ^ getRegister(cpu, REG_MBR));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
		break;

	case OP_AND:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			//initial read?
			case 3:
				clrRegister(cpu, REG_A);
				clrRegister(cpu, REG_MBR);
				break;
			case 7:
				setRegister(cpu, REG_A,
				            getRegister(cpu, REG_Z) & getRegister(cpu, REG_MBR));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
		break;

	case OP_IOR:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			//initial read?
			case 3:
				clrRegister(cpu, REG_A);
				clrRegister(cpu, REG_MBR);
				break;
			case 7:
				setRegister(cpu, REG_A,
				            getRegister(cpu, REG_Z) | getRegister(cpu, REG_MBR));
				setState(cpu, ST_EXECUTE); // NB: no execute state change in the book?
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
		break;

	case OP_NOT:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			//initial read?
			case 3:
				clrRegister(cpu, REG_A);
				clrRegister(cpu, REG_MBR);
				break;
			case 7:
				setRegister(cpu, REG_A, ~getRegister(cpu, REG_Z));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
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
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				setRegister(cpu, REG_DOL, getRegister(cpu, REG_A) >> 8 & 0xF000);
				setRegister(cpu, REG_DSL, getRegister(cpu, REG_A) & 0x003F);
				break;
			case 7:
				setSwitch(cpu, SW_TRA, true);
				break;
			case 8:
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			case 6:
				if (getSwitch(cpu, SW_READY) == true) {
					setSwitch(cpu, SW_TRA, false);
				}
				break;
			case 8:
				if (getSwitch(cpu, SW_TRA) == false) {
					setState(cpu, ST_FETCH);
					setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				}
				break;
			}
		}
		break;

	case OP_RAL:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_Z);
				break;
			case 7:
				setRegister(cpu, REG_Z, getRegister(cpu, REG_A));
				break;
			case 8:
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
				case 1:
					clrRegister(cpu, REG_A);
					break;
				case 2:
					setRegister(cpu, REG_A, 2 * getRegister(cpu, REG_Z));
					break;
				case 8:
					setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
					setState(cpu, ST_FETCH);
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
		break;
	}
}

