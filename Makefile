# Define the compiler
CC = cc

# Compiler flags (you can adjust as needed)
CFLAGS = -Wall -Wextra -g3 --std=gnu99 -Iinclude

all: bin/hello_world test bin/worker

# Build tools
bin/hello_world: src/hello_world.c
	$(CC) $(CFLAGS) -o bin/hello_world src/hello_world.c

bin/worker: src/worker.c
	$(CC) $(CFLAGS) -o bin/worker src/worker.c

# Build object files
test: test.c
	$(CC) $(CFLAGS) -o test test.c

run: src/maker.c clean
	 $(CC) $(CFLAGS) src/maker.c && ./a.out && ./bin/hello_world

# Clean up the build files
clean:
	rm -rf bin
	mkdir bin

.PHONY: clean run