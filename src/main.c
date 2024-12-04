/*
    Out of Order Apex CPU Implementation
*/
#include <stdio.h>

#include "instruction.h"
#include "cpu.h"

int main(int argc, char **argv) {

    printf("Hello, Apex Out of Order.\n");

    // TODO: Get the Instructions from args
    InstructionList inst_list = {0};

    // TODO: Simulate one cycle of the CPU
    Cpu cpu = {0};

    simulate_cycle(&cpu);

    return 0;
}