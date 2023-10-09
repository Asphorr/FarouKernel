#!/usr/bin/env bash

# Set up some variables
FUNICOD="funicod"
LIBNAME="lib$FUNICOD.a"
TESTEXE="test_$FUNICOD"
BENCHEXE="bench_$FUNICOD"
OBJFILES=$(ls | grep "\.o$")

# Remove object files
for objfile in $OBJFILES; do
  rm "$objfile"
done

# Remove executable
if [[ -x "$FUNICOD" ]]; then
  rm "$FUNICOD"
fi

# Remove library files
if [[ -e "$LIBNAME" ]]; then
  rm "$LIBNAME"
fi

# Remove test executable
if [[ -x "$TESTEXE" ]]; then
  rm "$TESTEXE"
fi

# Remove benchmark executable
if [[ -x "$BENCHEXE" ]]; then
  rm "$BENCHEXE"
fi

# Remove any remaining files
find . \( -name "*.dSYM" -or -name "*.DS_Store" \) -delete

# Remove empty directories
find . -type d -empty -exec rmdir {} \;
