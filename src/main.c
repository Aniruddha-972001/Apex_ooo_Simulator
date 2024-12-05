/*
    Out of Order Apex CPU Implementation
*/
#include <stdio.h>
#include <assert.h>

#include "instruction.h"
#include "cpu.h"

int main(int argc, char **argv) {

    printf("Hello, Apex Out of Order.\n");

    assert(argc == 2 && "Usage: ./cpu <asm_file>");

    InstructionList inst_list = parse(argv[1]);

    for (int i = 0; i < inst_list.len; i++) {
        print_instruction(inst_list.data[i]);
    }

    // TODO: Simulate one cycle of the CPU
    Cpu cpu = {0};

    simulate_cycle(&cpu);

    return 0;
}