#include "../include/cpu.h"

uint8_t clock_pulse = 0;
uint16_t RAM[RAM_LENGTH];

blue_register A;
blue_register DIL;
blue_register DOL;
blue_register DSL;
blue_register IR;
blue_register MAR;
blue_register MBR;
blue_register PC = 0x00;
blue_register SR;
blue_register Z;

State STATE = FETCH;

void cleanRam() {
	memset(RAM, 0x00, RAM_LENGTH * sizeof(uint16_t));
}

void emulateCycle() {
	for (clock_pulse = 0; clock_pulse < PULSE_AMT; clock_pulse += 1)
		processTick(clock_pulse);
}

void processTick(uint8_t tick) {
	switch (tick) {
	case 0:
		break;
	case 1:
		if (STATE == FETCH)
			PC += 1;
		break;
	case 2:
		if (STATE == FETCH)
			MBR = 0x00;
		break;
	case 3:
		if (STATE == FETCH) {
			IR = 0x00;
			MBR = RAM[MAR];
		}
		break;
	case 4:
		if (STATE == FETCH)
			IR = MBR;
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	default:
		break;
	}
	uint8_t INS = get_instruction();
	if (INS == 15) {
		do_NOP(tick);
	}
}

void dumpRegisters() {
	printf("PC: %04x A: %04x IR: %04x Z: %04x MAR: %04x MBR: %04x DSL: %02x DIL: %02x DOL: %02x\n",
	  PC, A, IR, Z, MAR, MBR, (DSL & 0x00FF), (DIL & 0x00FF), (DOL & 0x00FF));
}

void dumpMemory() {

}

uint8_t get_instruction() {
	return ((IR & 0xF000) >> 12);
}

void do_NOP(uint8_t tick) {
	if (tick == (PULSE_AMT - 1))
		MAR = PC;
}
