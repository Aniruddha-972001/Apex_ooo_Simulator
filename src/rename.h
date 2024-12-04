#pragma once

typedef struct {
    int table[32]; // Mapping from architectural registers to physical registers
    // TODO: Free list
} RenameTable;

// Maps given architectural register to a physical register
int map_source_register(RenameTable *rt, int arch);

// Maps given architectural destination register to a physical register
int map_dest_register(RenameTable *rt, int arch);

// Frees the physical register for reuse
void free_register(RenameTable *rt, int phy);