#pragma once

#include <stddef.h>

typedef struct
{
    int opcode; // Opcode of the instruction
    int rd;     // Destination Register
    int rs1;    // Source Register 1
    int rs2;    // Source Register 2
    int rs3;    // Source Register 3
    int imm;    // Immediate Value
} Instruction;

typedef struct
{
    size_t len, cap;
    Instruction *data;
} InstructionList;
