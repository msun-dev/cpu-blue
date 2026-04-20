#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PULSE_AMT 8
extern uint8_t clock_pulse;

#define RAM_LENGTH 4096
extern uint16_t RAM[RAM_LENGTH];

typedef uint16_t blue_register;
extern blue_register A;
extern blue_register DIL;
extern blue_register DOL;
extern blue_register DSL;
extern blue_register IR;
extern blue_register MAR;
extern blue_register MBR;
extern blue_register PC;
extern blue_register SR;
extern blue_register Z;

typedef enum State {
	EXECUTE,
	FETCH,
} State;
extern State STATE;

void cleanRam();

void emulateCycle();
void processTick(uint8_t tick);
void dumpRegisters();
void dumpMemory();
uint8_t get_instruction();

void do_NOP(uint8_t tick);
