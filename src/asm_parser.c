#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "asm_parser.h"

typedef struct
{
    size_t x, y;
} Span;

typedef struct
{
    char *value;
    Span span;
} Token;

typedef struct
{
    char *op;
    char **regs;
    size_t num_regs, line;
} InstructionToken;

typedef struct
{
    InstructionToken *data;
    size_t len, cap;
} InstructionTokenList;

void read_di(Instruction *inst, InstructionToken *it)
{
    char *reg, *imm;
    if (it->num_regs != 2)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires one destination register and one immediate value.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rd = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    imm = it->regs[1];
    if (imm[0] == '#')
    {
        inst->imm = atoi(imm + 1);
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be immediate value.\n", it->line, it->op);
        exit(1);
    }
}

void read_si(Instruction *inst, InstructionToken *it)
{
    char *reg, *imm;
    if (it->num_regs != 2)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires one source register and one immediate value.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rs1 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    imm = it->regs[1];
    if (imm[0] == '#')
    {
        inst->imm = atoi(imm + 1);
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be immediate value.\n", it->line, it->op);
        exit(1);
    }
}

void read_dsi(Instruction *inst, InstructionToken *it)
{
    char *reg, *imm;
    if (it->num_regs != 3)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires one destination register, one source register, and one immediate value.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rd = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[1];
    if (reg[0] == 'R')
    {
        inst->rs1 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be a register.\n", it->line, it->op);
        exit(1);
    }

    imm = it->regs[2];
    if (imm[0] == '#')
    {
        inst->imm = atoi(imm + 1);
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` third value must be immediate value.\n", it->line, it->op);
        exit(1);
    }
}

void read_dss(Instruction *inst, InstructionToken *it)
{
    char *reg;

    if (it->num_regs != 3)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires one destination register and two source registers.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rd = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[1];
    if (reg[0] == 'R')
    {
        inst->rs1 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[2];
    if (reg[0] == 'R')
    {
        inst->rs2 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` third value must be a register.\n", it->line, it->op);
        exit(1);
    }
}

void read_sss(Instruction *inst, InstructionToken *it)
{
    char *reg;

    if (it->num_regs != 3)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires three source registers.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rs1 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[1];
    if (reg[0] == 'R')
    {
        inst->rs2 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[2];
    if (reg[0] == 'R')
    {
        inst->rs3 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` third value must be a register.\n", it->line, it->op);
        exit(1);
    }
}

void read_ssi(Instruction *inst, InstructionToken *it)
{
    char *reg, *imm;

    if (it->num_regs != 3)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires two source register and one immediate value.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rd = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[1];
    if (reg[0] == 'R')
    {
        inst->rs1 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be a register.\n", it->line, it->op);
        exit(1);
    }

    imm = it->regs[2];
    if (imm[0] == '#')
    {
        inst->imm = atoi(imm + 1);
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` third value must be immediate value.\n", it->line, it->op);
        exit(1);
    }
}

void read_ss(Instruction *inst, InstructionToken *it)
{
    char *reg;

    if (it->num_regs != 2)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires two source registers.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rd = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[1];
    if (reg[0] == 'R')
    {
        inst->rs1 = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` second value must be a register.\n", it->line, it->op);
        exit(1);
    }
}

void read_s(Instruction *inst, InstructionToken *it)
{
    char *reg;

    if (it->num_regs != 1)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` requires one source register.\n", it->line, it->op);
        exit(1);
    }

    reg = it->regs[0];
    if (reg[0] == 'R')
    {
        inst->rd = atoi(reg + 1) - 1;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be a register.\n", it->line, it->op);
        exit(1);
    }
}

void read_i(Instruction *inst, InstructionToken *it)
{
    char *imm;

    if (it->num_regs != 1)
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` one immediate value.\n", it->line, it->op);
        exit(1);
    }

    imm = it->regs[0];
    if (imm[0] == '#')
    {
        inst->imm = atoi(imm + 1);
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: `%s` first value must be immediate value.\n", it->line, it->op);
        exit(1);
    }
}

Instruction parse_instruction(InstructionToken *it)
{
    Instruction instruction = {-1, -1, -1, -1, -1};
    if (strcmp(it->op, "ADD") == 0)
    {
        instruction.op = OPCODE_ADD;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "SUB") == 0)
    {
        instruction.op = OPCODE_SUB;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "MUL") == 0)
    {
        instruction.op = OPCODE_MUL;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "DIV") == 0)
    {
        instruction.op = OPCODE_DIV;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "AND") == 0)
    {
        instruction.op = OPCODE_AND;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "OR") == 0)
    {
        instruction.op = OPCODE_OR;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "XOR") == 0)
    {
        instruction.op = OPCODE_XOR;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "MOVC") == 0)
    {
        instruction.op = OPCODE_MOVC;
        read_di(&instruction, it);
    }
    else if (strcmp(it->op, "LOAD") == 0)
    {
        instruction.op = OPCODE_LOAD;
        read_dsi(&instruction, it);
    }
    else if (strcmp(it->op, "STORE") == 0)
    {
        instruction.op = OPCODE_STORE;
        read_ssi(&instruction, it);
    }
    else if (strcmp(it->op, "BZ") == 0)
    {
        instruction.op = OPCODE_BZ;
        read_i(&instruction, it);
    }
    else if (strcmp(it->op, "BNZ") == 0)
    {
        instruction.op = OPCODE_BNZ;
        read_i(&instruction, it);
    }
    else if (strcmp(it->op, "HALT") == 0)
    {
        instruction.op = OPCODE_HALT;
    }
    else if (strcmp(it->op, "ADDL") == 0)
    {
        instruction.op = OPCODE_ADDL;
        read_dsi(&instruction, it);
    }
    else if (strcmp(it->op, "SUBL") == 0)
    {
        instruction.op = OPCODE_SUBL;
        read_dsi(&instruction, it);
    }
    else if (strcmp(it->op, "LDR") == 0)
    {
        instruction.op = OPCODE_LDR;
        read_dss(&instruction, it);
    }
    else if (strcmp(it->op, "STR") == 0)
    {
        instruction.op = OPCODE_STR;
        read_sss(&instruction, it);
    }
    else if (strcmp(it->op, "CMP") == 0)
    {
        instruction.op = OPCODE_CMP;
        read_ss(&instruction, it);
    }
    else if (strcmp(it->op, "CML") == 0)
    {
        instruction.op = OPCODE_CML;
        read_si(&instruction, it);
    }
    else if (strcmp(it->op, "BP") == 0)
    {
        instruction.op = OPCODE_BP;
        read_i(&instruction, it);
    }
    else if (strcmp(it->op, "BN") == 0)
    {
        instruction.op = OPCODE_BN;
        read_i(&instruction, it);
    }
    else if (strcmp(it->op, "BNP") == 0)
    {
        instruction.op = OPCODE_BNP;
        read_i(&instruction, it);
    }
    else if (strcmp(it->op, "JUMP") == 0)
    {
        instruction.op = OPCODE_JUMP;
        read_si(&instruction, it);
    }
    else if (strcmp(it->op, "JALP") == 0)
    {
        instruction.op = OPCODE_JALP;
        read_di(&instruction, it);
    }
    else if (strcmp(it->op, "RET") == 0)
    {
        instruction.op = OPCODE_RET;
        read_s(&instruction, it);
    }
    else if (strcmp(it->op, "NOP") == 0)
    {
        instruction.op = OPCODE_NOP;
    }
    else
    {
        fprintf(stderr, "ERROR: Line %lu: Unknown opcode `%s`\n", it->line, it->op);
        exit(1);
    }

    return instruction;
}

void print_instruction(Instruction i)
{
    printf("Instruction { OP:%d D:%d S1:%d S2:%d S3:%d I:%d }\n", i.op, i.rd, i.rs1, i.rs2, i.rs3, i.imm);
}

void print_token(Token t)
{
    printf("Token { %s }\n", t.value);
}

char *read_file(char *file_name)
{
    char *buffer = NULL;
    long length;
    FILE *f = fopen(file_name, "rb");

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length + 1);
        if (buffer)
        {
            fread(buffer, 1, length, f);
            buffer[length] = '\0';
        }
        fclose(f);
    }
    return buffer;
}

void print_instruction_token(InstructionToken inst)
{
    printf("InstructionToken { %s ", inst.op);
    for (size_t i = 0; i < inst.num_regs; i++)
    {
        printf("%s ", inst.regs[i]);
    }
    printf("} on line: %lu\n", inst.line);
}

InstructionTokenList new_inst_tok_list()
{
    InstructionTokenList list;
    list.len = 0;
    list.cap = 2;
    list.data = malloc(list.cap * sizeof(InstructionToken));

    if (list.data == NULL)
    {
        printf("Failed to allocate InstructionTokenList\n");
        exit(1);
    }

    return list;
}

void add_instruction_token(InstructionTokenList *list, InstructionToken inst)
{
    if (list->len == list->cap)
    {
        list->cap *= 2;
        list->data = realloc(list->data, list->cap * sizeof(InstructionToken));
    }

    list->data[list->len] = inst;
    list->len += 1;
}

InstructionList new_inst_list()
{
    InstructionList list;
    list.len = 0;
    list.cap = 2;
    list.data = malloc(list.cap * sizeof(Instruction));

    if (list.data == NULL)
    {
        printf("Failed to allocate InstructionList\n");
        exit(1);
    }

    return list;
}

void add_instruction(InstructionList *list, Instruction inst)
{
    if (list->len == list->cap)
    {
        list->cap *= 2;
        list->data = realloc(list->data, list->cap * sizeof(Instruction));
    }

    list->data[list->len] = inst;
    list->len += 1;
}

int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

int is_alpha_or_digit(char c)
{
    return is_alpha(c) || is_digit(c);
}

// Assumes that src is not NULL
InstructionTokenList parse_assembly(char *src)
{
    size_t len = strlen(src);

    size_t line = 0;

    char acc[64];
    size_t acc_idx = 0;
    size_t token_start = 0;

    Token list[64];
    size_t list_idx = 0;

    InstructionTokenList code = new_inst_tok_list();

    for (size_t i = 0; i <= len; i++)
    {
        char c = src[i];
        if (is_alpha_or_digit(c) || c == '#')
        {
            // Store character into the token
            acc[acc_idx] = c;
            acc_idx += 1;

            if (acc_idx > 62)
            {
                acc[acc_idx] = '\0';
                printf("Error: Really long token enountered `%s...` on line: %lu\n", acc, line);
                exit(1);
            }
        }
        else if (c == '\n' || c == '\0' || c == '\t' || c == ' ' || c == '\r' || c == ',')
        {
            // End of token encountered
            acc[acc_idx] = '\0';

            // Don't add token to list if it is empty
            if (acc_idx > 0)
            {
                Token t;

                t.span.x = token_start;
                t.span.y = line;
                t.value = (char *)malloc(acc_idx);
                if (t.value == NULL)
                {
                    printf("Error: Failed to allocate token while parsing input.\n");
                    exit(1);
                }

                strcpy(t.value, acc);

                list[list_idx] = t;
                list_idx += 1;
            }

            if ((c == '\n' || c == '\0') && list_idx > 0)
            {
                // End of line encountered ... Parse the line into an instruction
                InstructionToken inst;

                inst.line = line + 1;
                inst.op = list[0].value;
                inst.regs = calloc(list_idx - 1, sizeof(char *));
                inst.num_regs = list_idx - 1;

                for (size_t j = 1; j < list_idx; j++)
                {
                    inst.regs[j - 1] = list[j].value;
                }

                add_instruction_token(&code, inst);

                line += 1;
                list_idx = 0;
            }

            acc_idx = 0;
        }
        else
        {
            // Invalid character found
            printf("Error: Invalid character `%c` found on line\n", c);
            exit(1);
        }
    }

    return code;
}

InstructionList parse(char *file_name)
{
    char *src = read_file(file_name);
    if (src == NULL)
    {
        printf("Failed to read from file.\n");
        exit(1);
    }
    InstructionTokenList code = parse_assembly(src);
    InstructionList list = new_inst_list();

    for (size_t i = 0; i < code.len; i++)
    {
        // print_instruction_token(code.data[i]);
        Instruction inst = parse_instruction(&code.data[i]);
        print_instruction(inst);
        add_instruction(&list, inst);
    }
    // TODO: Free InstructionTokenList
    free(src);
    return list;
}