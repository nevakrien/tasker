# Define the source file and output binary
SRC = src/hello_world.c
OUT = hello_world

# Define the compiler
CC = cc

# Compiler flags (you can adjust as needed)
CFLAGS = -Wall -Wextra -O2

# Build the binary
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

run: hello_world
	./hello_world

# Clean up the build files
clean:
	rm -f $(OUT)

.PHONY: clean run
