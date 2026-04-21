#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PULSE_AMT 8
#define RAM_LENGTH 4096
#define REGS_LENGTH 10

typedef enum State {
	EXECUTE,
	FETCH,
} State;

typedef enum Register {
	REG_PC  = 0,
	REG_IR  = 1,
	REG_MAR = 2,
	REG_MBR = 3,
	REG_DIL = 4,
	REG_DOL = 5,
	REG_DSL = 6,
	REG_A   = 7,
	REG_SR  = 8,
	REG_Z   = 9,
} Register;

typedef enum Instruction {
	OP_HLT = 0x0,
	OP_ADD = 0x1,
	OP_XOR = 0x2,
	OP_AND = 0x3,
	OP_IOR = 0x4,
	OP_NOT = 0x5,
	OP_LDA = 0x6,
	OP_STA = 0x7,
	OP_SRJ = 0x8,
	OP_JMA = 0x9,
	OP_JMP = 0xA,
	OP_INP = 0xB,
	OP_OUT = 0xC,
	OP_RAL = 0xD,
	OP_CSA = 0xE,
	OP_NOP = 0xF,
} Instruction;

typedef struct BlueCpu_t {
	uint8_t clock_pulse;
	uint16_t ram[RAM_LENGTH];
	State state;
	uint16_t registers[REGS_LENGTH];
} BlueCpu_t;

// Ready
BlueCpu_t* initCpu        ();
void       clearRam       (BlueCpu_t* cpu);
void       clearRegisters (BlueCpu_t* cpu);
void       loadProgramm   (BlueCpu_t* cpu, uint16_t* programm, uint32_t size);
// Process
void emulateCycle   (BlueCpu_t* cpu);
void processTick    (BlueCpu_t* cpu, uint8_t tick);
// Registers
void     setRegister (BlueCpu_t* cpu, Register r, uint16_t value);
uint16_t getRegister (BlueCpu_t* cpu, Register r);
void     incRegister (BlueCpu_t* cpu, Register r);
//void     decRegister (BlueCpu_t* cpu, Register r);
// Instructions
uint8_t getInstruction  (BlueCpu_t* cpu);
void    execInstruction (BlueCpu_t* cpu, Instruction instr, uint8_t tick);
// Debug
void dumpRegisters (BlueCpu_t* cpu);
void dumpMemory    (BlueCpu_t* cpu);

