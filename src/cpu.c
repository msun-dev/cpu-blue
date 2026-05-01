#include "../include/cpu.h"

#define VAL(x)    (x & 0x7FFF)
#define SIGN(x)   (x & 0x8000) // 1 = negative
#define INT16_MAX 0x7FFF
#define INT16_MIN 0x8000

// Secret functions
static void memcopy(uint16_t* dest, const uint16_t* src, uint16_t size) {
	// NB: uint16_t size for pointers. May fail if other types provided
	for (uint16_t i = 0; i < size; ++i) {
		dest[i] = src[i];
	}
}

static Bool detectOverflow(int16_t a, int16_t b) {
	if (SIGN(a) == SIGN(b)) {
		if (a > 0 && b > INT16_MAX - a) {
			return True;
		} else if (a < 0 && b < INT16_MIN - a) {
			return True;
		}
	}
	return False;
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
void setClockpulse (BlueCpu_t* cpu, uint8_t value) {
	cpu->clock_pulse = value;
}

uint8_t getClockpulse (BlueCpu_t* cpu) {
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
void setRamCell(BlueCpu_t* cpu, uint16_t addr, uint16_t value) {
	cpu->ram[addr & 0x0FFF] = value;
}

uint16_t getRamCell(BlueCpu_t* cpu, uint16_t addr) {
	if (addr > RAM_LEN) {
		disableCpu(cpu); // Yeah, just like that. Say thanks i'm not nuking your root
		return 0x0000;
	}
	return cpu->ram[addr];
}

void clearRam(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < RAM_LEN; setRamCell(cpu, i++, 0x0000));
}

// NB: Don't use yet!
void loadRam(BlueCpu_t* cpu, uint16_t* ram) {
	clearRam(cpu);
	memcopy(cpu->ram, ram, RAM_LEN);
}

uint8_t loadProgram(BlueCpu_t* cpu, uint16_t adr, uint16_t* program,
                                                  uint16_t size) {
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
void setRegister(BlueCpu_t* cpu, Register reg, uint16_t value) {
	cpu->registers[reg] = value;
}

uint16_t getRegister(BlueCpu_t* cpu, Register reg) {
	return cpu->registers[reg];
}

void clrRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, 0x0000);
}

void clearRegisters(BlueCpu_t* cpu) {
	for (uint32_t i = 0; i < REGS_LEN; clrRegister(cpu, i++));
}

void incRegister(BlueCpu_t* cpu, Register reg) {
	setRegister(cpu, reg, getRegister(cpu, reg) + 1);
}

// Process
uint8_t emulateCycle(BlueCpu_t* cpu) {
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
	uint16_t clock_pulse = getClockpulse(cpu);
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
uint8_t getInstruction(BlueCpu_t* cpu) {
	return ((getRegister(cpu, REG_IR) & 0xF000) >> 12);
}

void execInstruction(BlueCpu_t* cpu, uint8_t tick) {
	uint8_t cur_instr = getInstruction(cpu);
	switch (cur_instr) {
	case OP_HLT: // 0x0
		switch (tick) {
		case 7:
			setSwitch(cpu, SW_POWER, False);
			break;
		case 8:
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
			break;
		}
		break;
	
	case OP_ADD: // 0x1
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
				setRegister(cpu,
				            REG_MBR,
				            getRamCell(cpu, getRegister(cpu, REG_MAR)));
				break;
			case 7:;
				if (detectOverflow((int16_t)getRegister(cpu, REG_Z),
				                   (int16_t)getRegister(cpu, REG_MBR)) != 0) {
					setSwitch(cpu, SW_POWER, False);
					break;
				}
				int16_t r = (int16_t)getRegister(cpu, REG_Z) +
				            (int16_t)getRegister(cpu, REG_MBR);
				setRegister(cpu, REG_A, (uint16_t)r);
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
		break;
	
	case OP_XOR: // 0x2
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
			case 4:
				setRegister(cpu,
				            REG_MBR,
				            getRamCell(cpu, getRegister(cpu, REG_MAR)));
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
	
	case OP_AND: // 0x3
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
			case 4:
				setRegister(cpu,
				            REG_MBR,
				            getRamCell(cpu, getRegister(cpu, REG_MAR)));
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
	
	case OP_IOR: // 0x4
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
			case 4:
				setRegister(cpu,
				            REG_MBR,
				            getRamCell(cpu, getRegister(cpu, REG_MAR)));
				break;
			case 7:
				setRegister(cpu, REG_A,
				            getRegister(cpu, REG_Z) | getRegister(cpu, REG_MBR));
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				setState(cpu, ST_FETCH);
				break;
			}
		}
		break;
	
	case OP_NOT: // 0x5
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
	
	case OP_LDA: // 0x6
		if (getState(cpu) == ST_FETCH) {
			if (tick == 8) {
				setState(cpu, ST_EXECUTE);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
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
			case 5:;
				uint16_t ramonmar = getRamCell(cpu, getRegister(cpu, REG_MAR));
				setRegister(cpu, REG_MBR, ramonmar);
				setRegister(cpu, REG_A,   ramonmar);
				break;
			case 8:
				setState(cpu, ST_FETCH);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
			}
		}
		break;
	
	case OP_STA: // 0x7
		if (getState(cpu) == ST_FETCH) {
			if (tick == 8) {
				setState(cpu, ST_EXECUTE);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_IR) & 0x0FFF);
			}
		}
		else if (getState(cpu) == ST_EXECUTE) {
			switch (tick) {
			case 4:
				clrRegister(cpu, REG_MBR);
				break;
			case 5:
				setRegister(cpu, REG_MBR, getRegister(cpu, REG_A));
				setRamCell(cpu, getRegister(cpu, REG_MAR), getRegister(cpu, REG_MBR));
				break;
			case 8:
				setState(cpu, ST_FETCH);
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
			}
		}
		break;
	
	case OP_SRJ: // 0x8
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
	
	case OP_JMA: // 0x9
		switch (tick) {
			case 6:
				if ((getRegister(cpu, REG_A) & 0x8000))
					clrRegister(cpu, REG_PC);
				break;
			case 7:
				if ((getRegister(cpu, REG_A) & 0x8000))
					setRegister(cpu, REG_PC, getRegister(cpu, REG_IR) & 0x0FFF);
				break;
			case 8:
				setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
				break;
		}
		break;
	
	case OP_JMP: // 0xA
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
	
	case OP_RAL: // 0xD
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
					setRegister(cpu, REG_A,
					            ((getRegister(cpu, REG_Z) & 0x8000) >> 15 |
					             (getRegister(cpu, REG_Z) * 2)));
					break;
				case 8:
					setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
					setState(cpu, ST_FETCH);
					break;
			}
		}
		break;
	
	case OP_CSA: // 0xE
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
	
	case OP_NOP: // 0xF
		if (tick == 8) {
			setRegister(cpu, REG_MAR, getRegister(cpu, REG_PC));
		}
		break;
	default:
		break;
	}
}

