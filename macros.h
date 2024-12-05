#pragma once

#define DATA_MEMORY_SIZE 4096

/* Size of integer register file */
#define ARCH_REG_FILE_SIZE 32

/* Numeric OPCODE identifiers for instructions */
#define OPCODE_ADD 0x0
#define OPCODE_SUB 0x1
#define OPCODE_MUL 0x2
#define OPCODE_DIV 0x3
#define OPCODE_AND 0x4
#define OPCODE_OR 0x5
#define OPCODE_XOR 0x6
#define OPCODE_MOVC 0x7
#define OPCODE_LOAD 0x8
#define OPCODE_STORE 0x9
#define OPCODE_BZ 0xa
#define OPCODE_BNZ 0xb
#define OPCODE_HALT 0xc
#define OPCODE_ADDL 0xd
#define OPCODE_SUBL 0xe
#define OPCODE_LDR 0xf
#define OPCODE_STR 0x10
#define OPCODE_CMP 0x11
#define OPCODE_CML 0x12
#define OPCODE_BP 0x13
#define OPCODE_BN 0x14
#define OPCODE_BNP 0x15
#define OPCODE_JUMP 0x16
#define OPCODE_JALP 0x17
#define OPCODE_RET 0x18
#define OPCODE_NOP 0x19