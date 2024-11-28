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
bin/intrupt_cleanup.o: src/intrupt_cleanup.c
	$(CC) $(CFLAGS) -c -o bin/intrupt_cleanup.o src/intrupt_cleanup.c

# Build tools
bin/hello_world: src/hello_world.c
	$(CC) $(CFLAGS) -o bin/hello_world src/hello_world.c

bin/worker: src/worker.c
	$(CC) $(CFLAGS) -o bin/worker src/worker.c

# Build object files
test: test.c bin/intrupt_cleanup.o
	$(CC) $(CFLAGS) -o test test.c bin/intrupt_cleanup.o

run: test bin/worker
	./test 6969 4242
	rm .temp*

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
bin\intrupt_cleanup.o: src\intrupt_cleanup.c
	$(CC) $(CFLAGS) -c -o bin\intrupt_cleanup.o src\intrupt_cleanup.c

# Build tools
bin\hello_world.exe: src\hello_world.c
	$(CC) $(CFLAGS) -o bin\hello_world.exe src\hello_world.c

bin\worker.exe: src\worker.c
	$(CC) $(CFLAGS) -o bin\worker.exe src\worker.c

# Build object files
test.exe: test.c bin\intrupt_cleanup.o
	$(CC) $(CFLAGS) -o test.exe test.c bin\intrupt_cleanup.o

run: test.exe bin\worker.exe
	.\test.exe 6969 4242
	del /f /q .temp*

# Clean up the build files
clean:
	del /f /q test.exe
	rmdir /s /q bin
	mkdir bin

.PHONY: clean run

endif
