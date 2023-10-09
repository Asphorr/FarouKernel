#!/bin/bash

# Project settings
PROJECT_NAME="funicod"
TARGET_ARCHITECTURE="x86_64"

# Compiler settings
CC=gcc
CXX=g++
LD=ld

# Flags for C compiler
CFLAGS="-Wall -O3 -march=$TARGET_ARCHITECTURE -mtune=$TARGET_ARCHITECTURE -std=c99"

# Flags for C++ compiler
CXXFLAGS="-Wall -O3 -march=$TARGET_ARCHITECTURE -mtune=$TARGET_ARCHITECTURE -std=c++11"

# Libraries to link against
LIBS="-lpthread -lrt -lm"

# Source files
SOURCES=(main.c utils.c)

# Object files
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
