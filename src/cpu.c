#include <assert.h>

#include "macros.h"
#include "cpu.h"
#include "instruction.h"
#include "rename.h"

Cpu initialize_cpu(char *asm_file) {
    InstructionList inst_list = parse(asm_file); // We will need to free this later

    Cpu cpu = {0};
    cpu.code = inst_list;
    cpu.pc = 4000;
    cpu.rt = initialize_rename_table();

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

        cpu->pc += 4; // Go to next instruction
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

    // Renaming registers
    if (cpu->decode_2.inst.rd != -1) {
        int temp = cpu->decode_2.inst.rd;
        cpu->decode_2.inst.rd = map_dest_register(&cpu->rt, cpu->decode_2.inst.rd);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rd);
    }

    if (cpu->decode_2.inst.rs1 != -1) {
        int temp = cpu->decode_2.inst.rs1;
        cpu->decode_2.inst.rs1 = map_source_register(&cpu->rt, cpu->decode_2.inst.rs1);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs1);
    }
    if (cpu->decode_2.inst.rs2 != -1) {
        int temp = cpu->decode_2.inst.rs2;
        cpu->decode_2.inst.rs2 = map_source_register(&cpu->rt, cpu->decode_2.inst.rs2);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs2);
    }
    if (cpu->decode_2.inst.rs3 != -1) {
        int temp = cpu->decode_2.inst.rs3;
        cpu->decode_2.inst.rs3 = map_source_register(&cpu->rt, cpu->decode_2.inst.rs3);
        DBG("INFO", "Renamed Register R%d to P%d", temp, cpu->decode_2.inst.rs3);
    }
}

// Forwards data from each stage in the pipeline to the next stage
void forward_data(Cpu *cpu) {
 
    // Decode 1 -> Decode 2
    if (cpu->decode_1.has_inst) {
        cpu->decode_1.has_inst = false;

        cpu->decode_2.has_inst = true;
        cpu->decode_2.inst = cpu->decode_1.inst;
    }

    // Fetch -> Decode 1
    if (cpu->fetch.has_inst) {
        cpu->fetch.has_inst = false;

        cpu->decode_1.has_inst = true;
        cpu->decode_1.inst = cpu->fetch.inst;
    }
}

bool simulate_cycle(Cpu *cpu) {
    cpu->cycles += 1;
    DBG("\nINFO", "==================== Cycle %d ====================", cpu->cycles);

    // Fetch
    fetch(cpu);

    // Decode 1
    decode_1(cpu);

    // Decode 2
    decode_2(cpu);

    // Forward data to next stage
    forward_data(cpu);

    return false;
}