# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -ggdb

FILES = src/main.c src/cpu.c src/asm_parser.c src/rename.c src/rs.c src/rob.c src/util.c

cpu: $(FILES)
	$(CC) -o cpu $(FILES)

run: cpu
	./cpu.exe input/input.asm
