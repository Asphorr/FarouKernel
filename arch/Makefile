# Define the C compiler to use
CC = gcc

# Define any compile-time flags
CFLAGS = -Wall -g

# Define any directories containing header files
INCLUDES = -I./include

# Define library paths in addition to /usr/lib
LFLAGS = -L./lib

# Define any libraries to link into executable
LIBS = -lm

# Define the C source files
SRCS = $(wildcard ./src/*.c)

# Define the executable file 
MAIN = arch

.PHONY: clean

all: $(MAIN)
   @echo Program has been compiled

$(MAIN): $(OBJS) 
   $(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(SRCS) $(LFLAGS) $(LIBS)

clean:
   $(RM) *.o *~ $(MAIN)
