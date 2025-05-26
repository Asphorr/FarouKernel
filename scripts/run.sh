#!/bin/bash
set -e # Exit on first error

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
echo "Building the objects..."
$CC $CFLAGS ${SOURCES[*]} -o ${OBJECTS[*]}

# Build the executables
echo "Building the executables..."
$CXX $CXXFLAGS main.cpp utils.cpp -o $PROJECT_NAME

# Link the objects
echo "Linking the objects..."
$LD $OBJECTS $LIBS -o $PROJECT_NAME

# Run the executable
echo "Running the executable..."
if [ -f ./$PROJECT_NAME ]; then
    ./$PROJECT_NAME
else
    echo "Executable $PROJECT_NAME not found. Exiting."
    exit 1
fi

# Wait for the user to press Enter
read -p "Press Enter to continue... "

# Run the test cases
echo "Running test cases..."
if [ -f ./test_cases.sh ]; then
    ./test_cases.sh
else
    echo "Test cases script not found. Skipping."
fi

# Wait for the user to press Enter again
read -p "Press Enter to continue... "

# Run the benchmark
echo "Running benchmark..."
if [ -f ./benchmark.sh ]; then
    ./benchmark.sh
else
    echo "Benchmark script not found. Skipping."
fi

# Wait for the user to press Enter again
read -p "Press Enter to continue... "

# Clean up
echo "Cleaning up..."
trap 'rm -f ${OBJECTS[*]} $PROJECT_NAME' EXIT
