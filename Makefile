# Compiler setup.
CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

TARGETS=search_index

# Manually define all targets.
build: $(TARGETS)

# Compile all .c files into the search_index executable
$(TARGETS): $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) *.c -o $(TARGETS)

# Clean the solution.
clean:
	rm -f $(TARGETS) *.o

.PHONY: build clean