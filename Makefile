# Define the compiler
CC = cc

# Compiler flags (you can adjust as needed)
CFLAGS = -Wall -Wextra -O2 --std=c11 -Iinclude

all: hello_world bin/file_ops.o

# Build object files
hello_world: src/hello_world.c
	$(CC) $(CFLAGS) -o hello_world.o src/hello_world.c

bin/file_ops.o: src/file_ops.c
	$(CC) $(CFLAGS) -c -o file_ops.o src/file_ops.c

run: hello_world
	./hello_world

# Clean up the build files
clean:
	rm bin/*
	rm hello_world

.PHONY: clean run
