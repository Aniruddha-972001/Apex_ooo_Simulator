#pragma once
#include <stddef.h>

typedef struct
{
    int rs1;
    int rs2;
    int rs3;
    int rd;
    int imm;
    int op;
} Instruction;

typedef struct
{
    Instruction *data;
    size_t len, cap;
} InstructionList;

InstructionList parse(char *file_name);