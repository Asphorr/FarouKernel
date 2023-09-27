#!/bin/bash

# Remove object files
rm -f *.o

# Remove executable
rm -f ${funicod}

# Remove library files
rm -f lib${funicod}.a

# Remove test executable
rm -f test_${funicod}

# Remove benchmark executable
rm -f bench_${funicod}

# Remove any remaining files
rm -rf */*.dSYM
rm -rf */*.DS_Store

# Remove empty directories
rmdir --ignore-fail-on-non-empty */*/
