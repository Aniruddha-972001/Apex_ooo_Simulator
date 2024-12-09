#pragma once
#include <stddef.h>
#include "cpu_settings.h"

typedef struct {
    int data[FREE_LIST_CAPACITY];
    size_t len, head, tail;
} FreeList;

typedef struct {
    int table[ARCH_REGS_COUNT]; // Mapping from architectural registers to physical registers
    FreeList uprf_fl;

    int cc;                     // Mapping for CC register
    FreeList ucrf_fl;
} RenameTable;

RenameTable initialize_rename_table();

// Maps given architectural register to a physical register
int map_source_register(RenameTable *rt, int arch);

// Maps given architectural destination register to a physical register
int map_dest_register(RenameTable *rt, int arch);

// Remaps the current cc register to a new one
int map_cc_register(RenameTable *rt);

// Gets current mapping of cc register
int get_cc_register(RenameTable *rt);

void print_rename_table(RenameTable rt);
