/*
    Out of Order Apex CPU Implementation
*/
#include <stdio.h>
#include <assert.h>

#include "instruction.h"
#include "cpu.h"
#include "macros.h"

int main(int argc, char **argv) {

    printf("Hello, Apex Out of Order.\n\n");

    assert(argc == 2 && "Usage: ./cpu <asm_file>");

    Cpu cpu = initialize_cpu(argv[1]);

    DBG("INFO", "Instructions parsed: %lu", cpu.code.len);

    while (!simulate_cycle(&cpu));

    return 0;
}
