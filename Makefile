# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

FILES = src/main.c src/cpu.c

cpu: $(FILES)
	$(CC) -o cpu $(FILES)