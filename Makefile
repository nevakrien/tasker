# Define the compiler
CC = cc

# Compiler flags (you can adjust as needed)
CFLAGS = -Wall -Wextra -g3 --std=gnu99 -Iinclude

all: hello_world 

# Build object files
hello_world: src/hello_world.c
	$(CC) $(CFLAGS) -o bin/hello_world src/hello_world.c


run: src/maker.c clean
	 $(CC) $(CFLAGS) src/maker.c && ./a.out && ./bin/hello_world

# Clean up the build files
clean:
	rm -rf bin
	mkdir bin

.PHONY: clean run