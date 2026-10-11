# Compiler & Flags
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic -g
LDFLAGS =

# Target executable name
TARGET  = search_index

# Source files and Object files
SRCS    = main.c operations.c trie.c heap.c list_functions.c
OBJS    = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)

# Default target
all: build

# Build target
build: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile C source files into object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run in interactive mode
run: $(TARGET)
	./$(TARGET)

# Clean compiled objects and binaries
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe

.PHONY: all build run clean