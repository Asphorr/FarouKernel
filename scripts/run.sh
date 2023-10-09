#!/bin/bash

# Set the project name
PROJECT_NAME="funicod"

# Set the target architecture
TARGET_ARCHITECTURE="x86_64"

# Set the C compiler
CC=gcc

# Set the C++ compiler
CXX=g++

# Set the linker
LD=ld

# Set the flags for the C compiler
CFLAGS="-Wall -O2 -std=c99"

# Set the flags for the C++ compiler
CXXFLAGS="-Wall -O2 -std=c++11"

# Set the libraries to link against
LIBS="-lpthread -lrt -lm"

# Set the source files
SOURCES=(main.c utils.c)

# Set the object files
OBJECTS=(${SOURCES[@]:%.*}.o)

# Build the objects
$CC $CFLAGS ${SOURCES[*]} -o ${OBJECTS[*]}

# Build the executables
$CXX $CXXFLAGS main.cpp utils.cpp -o $PROJECT_NAME

# Link the objects
$LD $OBJECTS $LIBS -o $PROJECT_NAME

# Run the executable
./$PROJECT_NAME

# Wait for the user to press Enter
read -p "Press Enter to continue... "

# Run the test cases
echo "Running test cases..."
./test_cases.sh

# Wait for the user to press Enter again
read -p "Press Enter to continue... "

# Run the benchmark
echo "Running benchmark..."
./benchmark.sh

# Wait for the user to press Enter again
read -p "Press Enter to continue... "

# Clean up
rm -f ${OBJECTS[*]} $PROJECT_NAME
