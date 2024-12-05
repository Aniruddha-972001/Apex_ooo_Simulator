#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "asm_parser.c"


int main(int argc, char **argv) {
	if (argc != 2) {
		printf("ERROR: Please provide one .asm file.\n");
		return 1;
	}
	printf("File: %s\n", argv[1]);
	parse(argv[1]); 
    return 0;
}
