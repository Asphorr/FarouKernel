#!/bin/bash

# Download the latest release from GitHub using HTTPS
curl -L --insecure https://github.com/Asphorr/funiculous/releases/latest/download/funiculus.tar.gz | tar xzvf -C /tmp

# Verify the checksum of the downloaded archive
sha256sum funiculus.tar.gz > /dev/null || { echo "Checksum verification failed"; exit 1; }

# Move to the extracted directory
cd /tmp/funiculus*

# Compile the source files with address sanitizer and undefined behavior sanitizer
g++ -o funiculus -fsanitize=address -fsanitize=undefined -Wall -Wextra -pedantic -std=c++17 *.cpp

# Run the installed binary with AddressSanitizer and UBSAN
ASAN_OPTIONS=detect_leaks=0 ./funiculus

# Test for memory leaks
sudo malloc_history && { echo "Memory leak detected"; exit 1; }

# Test for buffer overflows
sudo valgrind --leak-check=full --track-origins=yes ./funiculus

# Test for integer overflows
sudo ./funiculus -i 1000000000

# Test for floating point exceptions
sudo ./funiculus -f 1.0e30

# Test for division by zero
sudo ./funiculus -d 0

# Test for out-of-range inputs
sudo ./funiculus -o 1000000000

# Test for invalid command line arguments
sudo ./funiculus -x 1

# Test for environment variable manipulation
export VARIABLE=123; sudo ./funiculus -e VARIABLE

# Clean up temporary files
rm -rf /tmp/funiculus*
