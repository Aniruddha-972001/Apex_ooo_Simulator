#pragma once

#include <stdbool.h>

#include "instruction.h"
#include "rename.h"
#include "rob.h"
#include "rs.h"

typedef struct {
    bool has_inst;
    Instruction inst;
} CpuStage;

typedef struct {
    bool has_inst;
    IQE  *iqe;
    int cycles;
} CpuFU;

typedef struct {
    InstructionList code;               // List of instructions

    int cycles;                         // Cycles counter
    int pc;                             // Program counter

    int memory[DATA_MEMORY_SIZE];       // Data memory

    int uprf_valid[PHYS_REGS_COUNT];    // UPRF valid bit
    int uprf[PHYS_REGS_COUNT];          // UPRF

    int fw_uprf_valid[PHYS_REGS_COUNT]; // Forwarded registers valid bits
    int fw_uprf[PHYS_REGS_COUNT];       // Forwarded registers 

    int ucrf_valid[CC_REGS_COUNT];      // UCRF Valid bit
    Cc  ucrf[CC_REGS_COUNT];            // UCRF

    int fw_ucrf_valid[CC_REGS_COUNT];   // Forwarded CC registers valid bits
    Cc fw_ucrf[CC_REGS_COUNT];          // Forwarded CC registers

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

    // Reorder Buffer
    Rob rob;
} Cpu;

Cpu initialize_cpu(char *asm_file);

// Updates dest with value of physical register if it is valid.
// returns 0 if physical register was invalid.
int get_urpf_value(Cpu cpu, int phy_reg, int *dest);

// Updates dest with value of cc register if it is valid.
// returns 0 if physical register was invalid.
int get_ucrf_value(Cpu cpu, int cc, Cc *dest);

// Simulates one cycle of the cpu
//
// Returns `true` if HALT instruction was completed
// Else returns `false`
bool simulate_cycle(Cpu *cpu);
