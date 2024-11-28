# Define the compiler
CC = cc

# Compiler flags
CFLAGS = -Wall -Wextra -g3 --std=gnu99 -Iinclude 

# Default to UNIX Makefile logic
ifndef OS
UNIX = 1
else
    ifneq ($(OS), Windows_NT)
    UNIX = 1
    endif
endif

# UNIX Makefile section
ifdef UNIX

all: bin/hello_world test bin/worker

# Objects
bin/global_sockets.o: src/global_sockets.c
	$(CC) $(CFLAGS) -c -o bin/global_sockets.o src/global_sockets.c

# Build tools
bin/hello_world: src/hello_world.c
	$(CC) $(CFLAGS) -o bin/hello_world src/hello_world.c

bin/worker: src/worker.c bin/global_sockets.o
	$(CC) $(CFLAGS) -o bin/worker src/worker.c bin/global_sockets.o

# Build object files
test: test.c bin/global_sockets.o
	$(CC) $(CFLAGS) -o test test.c bin/global_sockets.o

run: test bin/worker
	./test

# Clean up the build files
clean:
	rm test
	rm -rf bin
	mkdir bin

.PHONY: clean run

endif

# Windows Makefile section
ifndef UNIX

# Define the compiler for Windows
CC = zig cc

# Windows-specific libraries
WINLIBS = -lws2_32 -lIphlpapi
CFLAGS += $(WINLIBS)

all: bin\hello_world.exe test.exe bin\worker.exe

# Objects
bin\global_sockets.o: src\global_sockets.c
	$(CC) $(CFLAGS) -c -o bin\global_sockets.o src\global_sockets.c

# Build tools
bin\hello_world.exe: src\hello_world.c
	$(CC) $(CFLAGS) -o bin\hello_world.exe src\hello_world.c

bin\worker.exe: src\worker.c bin\global_sockets.o
	$(CC) $(CFLAGS) -o bin\worker.exe src\worker.c bin\global_sockets.o

# Build object files
test.exe: test.c bin\global_sockets.o
	$(CC) $(CFLAGS) -o test.exe test.c bin\global_sockets.o

run: test.exe bin\worker.exe
	.\test.exe

# Clean up the build files
clean:
	del /f /q test.exe
	rmdir /s /q bin
	mkdir bin

.PHONY: clean run

endif
