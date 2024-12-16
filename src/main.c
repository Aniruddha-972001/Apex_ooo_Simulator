/*
    Out of Order Apex CPU Implementation
*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "instruction.h"
#include "cpu.h"
#include "macros.h"
#include "rename.h"
#include "commands.h"
#include "util.h"
#define TRUE 1 

int get_command(char *input)
{
    char *token = strtok(input, " ");
    if (token == NULL) {
        return -1;
    }
    
    if (strcmp(token, STR_INITIALIZE) == 0) {
        return INITIALIZE;
    } else if (strcmp(token, STR_SINGLE_STEP) == 0) {
        return SINGLE_STEP;
    } else if (strcmp(token, STR_SIMULATE) == 0) {
        return SIMULATE;
    } else if (strcmp(token, STR_DISPLAY) == 0) {
        return DISPLAY;
    } else if (strcmp(token, STR_SHOW_MEM) == 0) {
        return SHOW_MEM;
    } else if (strcmp(token, STR_SET_MEM) == 0) {
        return SET_MEM;
    } else if (strcmp(token, STR_QUIT) == 0) {
        return QUIT;
    }

    return -1;
}

void repl(char *code_file)
{
    Cpu cpu;
    int is_done = 0;

    while (TRUE) {
        char line[100];
        printf("\nEnter command: ");
        fgets(line, sizeof(line), stdin);
        trim(line);
        printf("\n");
        
        int cmd = get_command(line);
        if (cmd == -1) {
            printf("Invalid command... Please try again. Or enter 'q' to Quit.\n");
            continue;
        }

        switch (cmd)
        {
        case INITIALIZE: {
                is_done = 0;
                cpu = initialize_cpu(code_file);
            }
            break;
        case SINGLE_STEP:
            {
                if (is_done) {
                    printf("Simulation completed. 'Initialize' again to restart. Or enter 'q' to Quit.\n");
                    break;
                }
                is_done = simulate_cycle(&cpu);
            } 
            break;
        case SIMULATE: {
                if (is_done) {
                    printf("Simulation completed. 'Initialize' again to restart. Or enter 'q' to Quit.\n");
                    break;
                }

                char *token = strtok(NULL, " "); // Number of simulations to perform
                if (token == NULL) {
                    printf("Usage: Simulate <n>\n");
                    break;
                }
                trim(token);

                int n = atoi(token);
                printf("Simulating %d cycles..\n", n);

                while (n > 0) {
                    is_done = simulate_cycle(&cpu);
                    n -= 1;

                    if (is_done) break; // Prematurely stop if simulation completed
                }
            }
            break;
        case DISPLAY:
            display(&cpu);
            break;
        case SHOW_MEM: {
                char *token = strtok(NULL, " "); // Number of simulations to perform
                if (token == NULL) {
                    printf("Usage: ShowMem <address>\n");
                    break;
                }
                trim(token);

                int address = atoi(token);

                show_mem(&cpu, address);
            }
            break;
        case SET_MEM: {
                char *token = strtok(NULL, " "); // Number of simulations to perform
                if (token == NULL) {
                    printf("Usage: SetMem <filename>\n");
                    break;
                }
                trim(token);

                set_memory(&cpu, token);
            }
            break;
        case QUIT:
            goto done;
            break;
        
        default:
            continue;
        }
    }
done:
    printf("Simulation completed...\n");
    return;
}

int main(int argc, char **argv) {

    printf("Hello, Apex Out of Order.\n\n");

    assert(argc == 2 && "Usage: ./cpu <asm_file>");

    Cpu cpu = initialize_cpu(argv[1]);

    DBG("INFO", "Instructions parsed: %lu", cpu.code.len);

    //while (!simulate_cycle(&cpu));
    // for (int i = 0; i < 2; i++) simulate_cycle(&cpu);
    repl(argv[1]);
    printf("Current simulation lasted for %d cycles.\n", cpu.cycles);

    return 0;
}