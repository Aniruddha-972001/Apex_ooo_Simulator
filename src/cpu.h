#pragma once

#include <stdbool.h>

#include "instruction.h"
#include "rename.h"
#include "rs.h"

typedef struct {
    bool has_inst;
    Instruction inst; 
} CpuStage;

typedef struct {
    bool has_inst;
    IQE  iqe;
} CpuFU;

typedef struct {
    InstructionList code;               // List of instructions

    int cycles;                         // Cycles counter
    int pc;                             // Program counter

    int uprf_valid[PHYS_REGS_COUNT];    // UPRF valid bit
    int uprf[PHYS_REGS_COUNT];          // UPRF
    RenameTable rt;                     // RenameTable and FreeList

    // Reservation Stations
    IRS irs;
    LSQ lsq;
    MRS mrs;

    // Stages
    CpuStage fetch;
    CpuStage decode_1;
    CpuStage decode_2;

    // Functional Units
    CpuFU intFU;
    CpuFU mulFU;
    CpuFU memFU;
} Cpu;

Cpu initialize_cpu(char *asm_file);

// Updates dest with value of physical register if it is valid.
// returns 0 if physical register was invalid.
int get_urpf_value(Cpu cpu, int phy_reg, int *dest);

// Simulates one cycle of the cpu
//
// Returns `true` if HALT instruction was completed
// Else returns `false`
bool simulate_cycle(Cpu *cpu);