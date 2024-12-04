# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

FILES = src/main.c

cpu: $(FILES)
	$(CC) -o cpu $(FILES)