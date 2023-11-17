# Define the C++ compiler to use
CC = g++

# Define any compile-time flags
CFLAGS = -Wall -g

# Define any directories containing header files
INCLUDES = 

# Define library paths in addition to /usr/lib
LFLAGS = 

# Define any libraries to link into executable
LIBS = 

# Define the C++ source files
SRCS = x86_64.cpp

# Define the executable file 
MAIN = x86_64

.PHONY: clean

all: $(MAIN)
   @echo Program has been compiled

$(MAIN): $(OBJS) 
   $(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(SRCS) $(LFLAGS) $(LIBS)

clean:
   $(RM) *.o *~ $(MAIN)
