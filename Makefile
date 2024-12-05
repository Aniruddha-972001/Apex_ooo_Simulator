# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

FILES = src/main.c src/cpu.c src/asm_parser.c

cpu: $(FILES)
	$(CC) -o cpu $(FILES)

run: cpu
	./cpu.exe input/input.asm