#pragma once

#include <stddef.h>

typedef struct
{
    int op;     // Opcode of the instruction
    int rd;     // Destination Register
    int rs1;    // Source Register 1
    int rs2;    // Source Register 2
    int rs3;    // Source Register 3
    int imm;    // Immediate Value
    int cc;     // cc used by this inst
} Instruction;

typedef struct
{
    size_t len, cap;
    Instruction *data;
} InstructionList;

InstructionList parse(char *file_name);
char *get_op_name(int opcode);
void print_instruction(Instruction i);

/* Numeric OPCODE identifiers for instructions */
#define OP_ADD 0x0
#define OP_SUB 0x1
#define OP_MUL 0x2
#define OP_DIV 0x3
#define OP_AND 0x4
#define OP_OR 0x5
#define OP_XOR 0x6
#define OP_MOVC 0x7
#define OP_LOAD 0x8
#define OP_STORE 0x9
#define OP_BZ 0xa
#define OP_BNZ 0xb
#define OP_HALT 0xc
#define OP_ADDL 0xd
#define OP_SUBL 0xe
#define OP_LDR 0xf
#define OP_STR 0x10
#define OP_CMP 0x11
#define OP_CML 0x12
#define OP_BP 0x13
#define OP_BN 0x14
#define OP_BNP 0x15
#define OP_JUMP 0x16
#define OP_JALP 0x17
#define OP_RET 0x18
#define OP_NOP 0x19
