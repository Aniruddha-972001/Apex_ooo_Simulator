#pragma once

#include <stdbool.h>

#include "instruction.h"
#include "rename.h"

typedef struct {
    bool has_inst;
    Instruction inst; 
} CpuStage;

typedef struct {
    InstructionList code; // List of instructions

    int cycles; // Cycles counter
    int pc; // Program counter
    RenameTable rt; // RenameTable and FreeList

    CpuStage fetch;
    CpuStage decode_1;
    CpuStage decode_2;
} Cpu;

Cpu initialize_cpu(char *asm_file);

// Simulates one cycle of the cpu
//
// Returns `true` if HALT instruction was completed
// Else returns `false`
bool simulate_cycle(Cpu *cpu);