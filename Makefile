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
	$(CC) $(CFLAGS) -otest test.c

run: test bin/worker
	 ./test 6969 4242
	 rm .temp*

# Clean up the build files
clean:
	rm test
	rm -rf bin
	mkdir bin

.PHONY: clean run