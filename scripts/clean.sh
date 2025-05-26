#!/usr/bin/env bash

# Define functions for removing various types of files
remove_object_files() {
  local objfiles
  objfiles=$(ls | grep "\.o$")
  if [[ -n "$objfiles" ]]; then
    echo "Removing object files:"
    printf "%s\n" "${objfiles[@]}"
    rm --force "${objfiles[@]}"
  fi
}

remove_executable() {
  local funicod
  funicod="$1"
  if [[ -x "$funicod" ]]; then
    echo "Removing executable '$funicod'"
    rm --force "$funicod"
  fi
}

remove_library_files() {
  local libname
  libname="$1"
  if [[ -e "$libname" ]]; then
    echo "Removing library file '$libname'"
    rm --force "$libname"
  fi
}

remove_test_exe() {
  local testexe
  testexe="$1"
  if [[ -x "$testexe" ]]; then
    echo "Removing test executable '$testexe'"
    rm --force "$testexe"
  fi
}

remove_bench_exe() {
  local benchexe
  benchexe="$1"
  if [[ -x "$benchexe" ]]; then
    echo "Removing benchmark executable '$benchexe'"
    rm --force "$benchexe"
  fi
}

remove_dsym_and_store() {
  find . \( -name "*.dSYM" -or -name "*.DS_Store" \) -delete
}

remove_empty_dirs() {
  find . -type d -empty -exec rmdir {} \;
}

# Main function
main() {
  # Set up some variables
  funicod="funicod"
  libname="lib$funicod.a"
  testexe="test_$funicod"
  benchexe="bench_$funicod"

  # Call functions to remove various types of files
  remove_object_files
  remove_executable "$funicod"
  remove_library_files "$libname"
  remove_test_exe "$testexe"
  remove_bench_exe "$benchexe"
  remove_dsym_and_store
  remove_empty_dirs
}

# Call main function
main
