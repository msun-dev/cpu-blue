#pragma once

#include <string.h> // memcpy
#include <stdlib.h> // malloc

#define PULSE_AMT 8
#define RAM_LEN 4096
#define REGS_LEN 10
#define SWITCHES_LEN 3

#define uint8_t  unsigned char
#define uint16_t unsigned short int
#define uint32_t unsigned int

typedef enum Bool { // NB: Care with cond statements, can fail!
	False = 0,
	True  = !False,
} Bool;

typedef enum State {
	ST_EXECUTE,
	ST_FETCH,
} State;

typedef enum Register {
	REG_PC  = 0, // program counter
	REG_IR  = 1, // current instruction
	REG_MAR = 2, // Memory Address Register (adress to read)
	REG_MBR = 3, // Memory Buffer Register (data from MAR)
	REG_DIL = 4, // Input from a device
	REG_DOL = 5, // Output to a device
	REG_DSL = 6, // Device num?
	REG_A   = 7, // accumulator A
	REG_SR  = 8, // ?
	REG_Z   = 9, // accumulator Z (unavailable to User)
} Register;

typedef enum Instruction {
	OP_HLT = 0x0,
	OP_ADD = 0x1,
	OP_XOR = 0x2,
	OP_AND = 0x3,
	OP_IOR = 0x4,
	OP_NOT = 0x5,
	OP_LDA = 0x6, // Load Data to A from XXXX
	OP_STA = 0x7, // STore from A to XXXX
	OP_SRJ = 0x8, // Sub Routine Jump
	OP_JMA = 0x9, // if A sign == 1: jump to XXXX
	OP_JMP = 0xA,
	OP_INP = 0xB, // 8 bits written to A from device **YY
	OP_OUT = 0xC, // 8 bits of A sent to device **YY
	OP_RAL = 0xD, // Accumulator rotation
	OP_CSA = 0xE, // REG_switch -> A
	OP_NOP = 0xF,
} Instruction;

typedef enum Switch {
	SW_POWER = 0, // "Is this thing on?"
	SW_READY = 1, // Ready to read data from a device
	SW_TRA   = 2, // Data transfer in progress
} Switch;

typedef struct BlueCpu_t {
	uint8_t clock_pulse;
	State   state;

	uint16_t ram[RAM_LEN];
	uint16_t registers[REGS_LEN];
	Bool     status_switches[SWITCHES_LEN];
} BlueCpu_t;

// Initialisation
BlueCpu_t* initCpu        ();
void       loadRam        (BlueCpu_t* cpu, uint16_t* ram);
uint8_t    loadProgramm   (BlueCpu_t* cpu, uint16_t adr,
                           uint16_t* programm, uint16_t size);
void       clearRam       (BlueCpu_t* cpu);
void       clearRegisters (BlueCpu_t* cpu);
void       deinitCpu      (BlueCpu_t* cpu);
// General data
void    setClockpulse (BlueCpu_t* cpu, uint8_t value);
uint8_t getClockpulse (BlueCpu_t* cpu);
void    incClockpulse (BlueCpu_t* cpu);
void  setState (BlueCpu_t* cpu, State s);
State getState (BlueCpu_t* cpu);
// Switches
void setSwitch  (BlueCpu_t* cpu, Switch sw, Bool value);
Bool getSwitch  (BlueCpu_t* cpu, Switch sw);
void enableCpu  (BlueCpu_t* cpu);
void disableCpu (BlueCpu_t* cpu);
// Registers
void     setRegister (BlueCpu_t* cpu, Register r, uint16_t value);
uint16_t getRegister (BlueCpu_t* cpu, Register r);
void     clrRegister (BlueCpu_t* cpu, Register r);
void     incRegister (BlueCpu_t* cpu, Register r);
// Process
uint8_t emulateCycle (BlueCpu_t* cpu);
void    processTick  (BlueCpu_t* cpu);
// Instructions
uint8_t getInstruction  (BlueCpu_t* cpu);
void    execInstruction (BlueCpu_t* cpu, uint8_t tick);

