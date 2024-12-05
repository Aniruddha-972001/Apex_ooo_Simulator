#include <assert.h>

#include "macros.h"
#include "cpu.h"
#include "instruction.h"

Cpu initialize_cpu(char *asm_file) {
    InstructionList inst_list = parse(asm_file);

    Cpu cpu = {0};
    cpu.code = inst_list;
    cpu.pc = 4000;

    return cpu;
}

// Convert pc from address space to index in instruction list
int pc_to_index(int pc) {
    return (pc - 4000) / 4;
}

// Fetch stage
void fetch(Cpu *cpu) {
    int index = pc_to_index(cpu->pc);

    if (index >= 0 && index < cpu->code.len) {
        Instruction inst = cpu->code.data[index];

        cpu->fetch.has_inst = true;
        cpu->fetch.inst = inst;
    } else {
        cpu->fetch.has_inst = false;

        DBG("WARN", "Invalid program counter: %d (index = %d)", cpu->pc, index);
    }
}

void decode_1(Cpu *cpu) {
    if (!cpu->decode_1.has_inst) return;

    // Currently Decode 1 does nothing
}

void decode_2(Cpu *cpu) {
    if (!cpu->decode_2.has_inst) return;

    // TODO: Register renaming
}

bool simulate_cycle(Cpu *cpu) {
    // Fetch
    fetch(cpu);

    // Decode 1
    decode_1(cpu);

    // Decode 2
    decode_2(cpu);


    return false;
}