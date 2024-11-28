# Define the compiler
CC = cc

#windows needs -lws2_32 -lIphlpapi
# Compiler flags (you can adjust as needed)
CFLAGS = -Wall -Wextra -g3 --std=gnu99 -Iinclude 

all: bin/hello_world test bin/worker

#objects
bin/intrupt_cleanup.o: src/intrupt_cleanup.c
	$(CC) $(CFLAGS) -c -o bin/intrupt_cleanup.o src/intrupt_cleanup.c

# Build tools
bin/hello_world: src/hello_world.c
	$(CC) $(CFLAGS) -o bin/hello_world src/hello_world.c

bin/worker: src/worker.c
	$(CC) $(CFLAGS) -o bin/worker src/worker.c

# Build object files
test: test.c bin/intrupt_cleanup.o
	$(CC) $(CFLAGS) -otest test.c bin/intrupt_cleanup.o

run: test bin/worker
	 ./test 6969 4242
	 rm .temp*

# Clean up the build files
clean:
	rm test
	rm -rf bin
	mkdir bin

.PHONY: clean run