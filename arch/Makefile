# Define the compiler and flags
CC = gcc
CFLAGS = -m64 -Wall -Wextra -O2

# Define the object files
OBJS = setup_64.o head_64.o entry_64.o

# Define the target
TARGET = kernel

# Define the dependencies
DEPS = setup_64.h head64.h processor.h desc.h pgtable.h pgtable-64.h pgtable-3level_types.h pgtable-3level.h pgtable-2level_types.h pgtable-2level.h

# Define the rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET)

# Include the dependency files
-include $(DEPS)
