#include "../include/cpu.h"

// Secret functions
static void memcopy(ui16_t* dest, const ui16_t* src, s_t size) {
	// NB: ui16_t size for pointers. May fail if other types provided
	for (s_t i = 0; i < size; ++i) {
		dest[i] = src[i];
	}
}

static ui8_t detectOverflow(ui16_t Z, ui16_t MBR) {
	ui32_t result = Z + MBR;
	if ((Z & 0x8000) && (MBR & 0x8000) && !(result & 0x8000))
		return 1;
	else if (!(Z & 0x8000) && !(MBR & 0x8000) && (result & 0x8000))
		return 2;
	return 0;
}

// Initialisation
BlueCpu_t* initCpu(AllocFunc_t allocFunc, FreeFunc_t freeFunc) {
	// NB: For now only tested agains malloc
	BlueCpu_t* cpu = NULL;
	cpu = (BlueCpu_t*)allocFunc(sizeof(BlueCpu_t));
	if (!cpu) {
		freeFunc(cpu);
		return NULL;
	}

	setClockpulse(cpu, 1);
	setState(cpu, ST_FETCH);
	clearRam(cpu);
	clearRegisters(cpu);
	setSwitch(cpu, SW_POWER, False);
	setSwitch(cpu, SW_READY, False);
	setSwitch(cpu, SW_TRA, False);

	return cpu;
}

void deinitCpu(BlueCpu_t* cpu, FreeFunc_t freeFunc) {
	freeFunc(cpu);
}

// General data
void setClockpulse (BlueCpu_t* cpu, ui8_t value) {
	cpu->clock_pulse = value;
}

ui8_t getClockpulse (BlueCpu_t* cpu) {
	return cpu->clock_pulse;
}

void incClockpulse (BlueCpu_t* cpu) {
	cpu->clock_pulse++;
}

void setState(BlueCpu_t* cpu, State s) {
	cpu->state = s;
}

State getState(BlueCpu_t* cpu) {
	return cpu->state;
}

// Ram
void setRamCell(BlueCpu_t* cpu, ui16_t addr, ui16_t value) {
	if (addr > RAM_LEN)
		return;
	cpu->ram[addr] = value;
}

ui16_t getRamCell(BlueCpu_t* cpu, ui16_t addr) {
	if (addr > RAM_LEN) {
		disableCpu(cpu); // Yeah, just like that. Say thanks i'm not nuking your root
		return 0x0000;
	}
	return cpu->ram[addr];
}

void clearRam(BlueCpu_t* cpu) {
	for (ui32_t i = 0; i < RAM_LEN; setRamCell(cpu, i++, 0x0000));
}

void loadRam(BlueCpu_t* cpu, ui16_t* ram) {
	clearRam(cpu);
	memcopy(cpu->ram, ram, RAM_LEN);
}

ui8_t loadProgram(BlueCpu_t* cpu, ui16_t adr, ui16_t* program, ui16_t size) {
	clearRam(cpu);
	if ((adr + size) > RAM_LEN)
		return 1;
	memcopy(cpu->ram + adr, program, size);
	return 0;
}

// Switches
void setSwitch(BlueCpu_t* cpu, Switch sw, Bool value) {
	cpu->status_switches[sw] = value;
}

Bool getSwitch(BlueCpu_t* cpu, Switch sw) {
	return cpu->status_switches[sw];
}

void enableCpu(BlueCpu_t* cpu) {
	setSwitch(cpu, SW_POWER, True);
}

void disableCpu (BlueCpu_t* cpu) {
	setSwitch(cpu, SW_POWER, False);
}

// Registers
void setRegister(BlueCpu_t* cpu, Register reg, ui16_t value) {
	cpu->registers[reg] = value;
}

ui16_t getRegister(BlueCpu_t* cpu, Register reg) {
	return cpu->registers[reg];
}

void clrRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, 0x0000);
}

void clearRegisters(BlueCpu_t* cpu) {
	for (ui32_t i = 0; i < REGS_LEN; clrRegister(cpu, i++));
}

void incRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, getRegister(cpu, reg) + 1);
}

// Process
ui8_t emulateCycle(BlueCpu_t* cpu) {
	if (getSwitch(cpu, SW_POWER) == False) {
		return 1;
	}

	for (setClockpulse(cpu, 1);
	     getClockpulse(cpu) < PULSE_AMT + 1;
	     incClockpulse(cpu)) {
		processTick(cpu);
	}

	return 0;
}

void processTick(BlueCpu_t* cpu) {
	ui16_t clock_pulse = getClockpulse(cpu);
	// Move if state here
	switch (clock_pulse) {
	case 1:
		break;
	case 2:
		if (getState(cpu) == ST_FETCH)
			incRegister(cpu, REG_PC);
		break;
	case 3:
		if (getState(cpu) == ST_FETCH)
			clrRegister(cpu, REG_MBR);
		break;
	case 4:
		if (getState(cpu) == ST_FETCH) {
			clrRegister(cpu, REG_IR);
			setRegister(cpu, REG_MBR, getRamCell(cpu, getRegister(cpu, REG_MAR)));
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
	execInstruction(cpu, clock_pulse);
}

// Instructions
ui8_t getInstruction(BlueCpu_t* cpu) {
	return ((getRegister(cpu, REG_IR) & 0xF000) >> 12);
}

void execInstruction(BlueCpu_t* cpu, ui8_t tick) {
	ui8_t cur_instr = getInstruction(cpu);
	switch (cur_instr) {
	case OP_HLT:
		switch (tick) {
		case 7:
			setSwitch(cpu, SW_POWER, False);
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
				// loading data (12 bits) from IR (was MBR) to MAR
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
			case 4:
				setRegister(cpu, REG_MBR, getRegister(cpu, REG_MAR));
				break;
			case 7:;
				ui32_t result = getRegister(cpu, REG_Z) + getRegister(cpu, REG_MBR);
				if (detectOverflow(
				      getRegister(cpu, REG_Z),
				      getRegister(cpu, REG_MBR)
				  ) != 0) {
					setSwitch(cpu, SW_POWER, False);
				}
				setRegister(cpu, REG_A, (ui16_t)result);
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
		if (getState(cpu) == ST_FETCH) {
			if (tick == 8) {
				setState(cpu, ST_EXECUTE);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x8FFF);
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			case 2:
				clrRegister(cpu, REG_A);
				break;
			case 3:
				clrRegister(cpu, REG_MBR);
				break;
			case 5:
				setRegister(cpu, REG_A, getRegister(cpu, REG_MBR) & 0x8FFF);
				break;
			case 8:
				setState(cpu, ST_FETCH);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
			}
		}
		break;

	case OP_STA:
		if (getState(cpu) == ST_FETCH) {
			if (tick == 8) {
				setState(cpu, ST_EXECUTE);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x8FFF);
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			case 4:
				clrRegister(cpu, REG_MBR);
				break;
			case 5:
				setRegister(cpu, REG_MBR, getRegister(cpu, REG_A));
				break;
			case 8:
				setState(cpu, ST_FETCH);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
			}
		}
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
				if ((getRegister(cpu, REG_A) & 0x8000)) // NB: Can be wrong
					clrRegister(cpu, REG_PC);
				break;
			case 7:
				if ((getRegister(cpu, REG_A) & 0x8000)) // NB: Can be wrong
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

	case OP_INP:
		// NB: both OP_INP and OP_OUT expected to not work
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				clrRegister(cpu, REG_A);
				setRegister(cpu, REG_DSL, getRegister(cpu, REG_IR) & 0x001F);
				break;
			case 7:
				setSwitch(cpu, SW_TRA, True);
				break;
			case 8:
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			case 6:
				if (getSwitch(cpu, SW_READY) == True) {
					setSwitch(cpu, SW_TRA, False);
				}
				break;
			case 8:
				if (getSwitch(cpu, SW_TRA) == False) {
					setState(cpu, ST_FETCH);
					setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				}
				break;
			}
		}
		break;

	case OP_OUT:
		if (getState(cpu) == ST_FETCH) {
			switch (tick) {
			case 6:
				setRegister(cpu, REG_DOL, getRegister(cpu, REG_A) & 0xFF00); // 15-8 REG_A
				setRegister(cpu, REG_DSL, getRegister(cpu, REG_IR) & 0x003F); // 5-0 REG_A
				break;
			case 7:
				setSwitch(cpu, SW_TRA, True);
				break;
			case 8:
				setState(cpu, ST_EXECUTE);
				break;
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			case 6:
				if (getSwitch(cpu, SW_READY) == True) {
					setSwitch(cpu, SW_TRA, False);
				}
				break;
			case 8:
				if (getSwitch(cpu, SW_TRA) == False) {
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

