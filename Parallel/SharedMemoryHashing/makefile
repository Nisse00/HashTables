# Variables
CC = gcc
CFLAGS = -std=c11 -pthread -Wall -Wextra -g
OBJ = atomic_update.o hashtable.o main.o my_element.o
TARGET = main_program

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile each .c file into .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean
