#!/usr/bin/env bash

# Exit on error
set -e

# Project configuration
PROJECT="funicod"
VERSION="1.0.0"
ARCH="x86_64"
BUILD_DIR="build"
OUTPUT_DIR="dist"

# Source files
C_SOURCES=("main.c" "utils.c")
CPP_SOURCES=("main.cpp" "utils.cpp")

# Compiler settings (respect environment variables if set)
CC=${CC:-"gcc"}
CXX=${CXX:-"g++"}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 1)}

# Parse command line arguments
BUILD_TYPE="release"
BUILD_PACKAGE=true
BUILD_RUN=true
BUILD_CLEAN=false

# Configure flags based on build type
if [[ "$BUILD_TYPE" == "debug" ]]; then
    BASE_FLAGS="-Wall -Wextra -g -O0 -DDEBUG"
else
    BASE_FLAGS="-Wall -Wextra -O3 -march=$ARCH -mtune=$ARCH -pipe -flto 
                -funroll-loops -finline-functions -ffunction-sections 
                -fdata-sections -Wl,--gc-sections"
fi

export CFLAGS="$BASE_FLAGS"
export CXXFLAGS="$BASE_FLAGS -std=c++17"
export LDFLAGS="-lpthread -lrt -lm"

# Create build directories
mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

# Build function
build() {
    echo "Building $PROJECT v$VERSION ($BUILD_TYPE) using $JOBS jobs..."
    
    # Compile C sources
    for src in "${C_SOURCES[@]}"; do
        [[ -f "$src" ]] && $CC $CFLAGS -c "$src" -o "$BUILD_DIR/$(basename "${src%.c}.o")"
    done
    
    # Compile C++ sources
    for src in "${CPP_SOURCES[@]}"; do
        [[ -f "$src" ]] && $CXX $CXXFLAGS -c "$src" -o "$BUILD_DIR/$(basename "${src%.cpp}.o")"
    done
    
    # Link
    $CXX $CXXFLAGS $BUILD_DIR/*.o $LDFLAGS -o "$OUTPUT_DIR/$PROJECT"
    
    # Optimize binary
    strip --strip-all "$OUTPUT_DIR/$PROJECT"
    
    # Compress binary (if UPX is installed)
    if command -v upx >/dev/null 2>&1; then
        upx -9 "$OUTPUT_DIR/$PROJECT"
    fi
}

# Package function
package() {
    echo "Packaging $PROJECT v$VERSION..."
    
    mkdir -p "$OUTPUT_DIR"
    tar czf "$OUTPUT_DIR/$PROJECT-$VERSION.tgz" -C "$OUTPUT_DIR" "$PROJECT" \
        $([ -f README.md ] && echo README.md) $([ -f LICENSE ] && echo LICENSE)
    
    echo "Package created: $OUTPUT_DIR/$PROJECT-$VERSION.tgz"
}

# Clean function
clean() {
    echo "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"/* "$OUTPUT_DIR"/*
}

# Main execution
[[ "$BUILD_CLEAN" == true ]] && clean
build
[[ "$BUILD_PACKAGE" == true ]] && package

# Run the program
if [[ "$BUILD_RUN" == true ]]; then
    echo "Running $PROJECT..."
    "$OUTPUT_DIR/$PROJECT"
fi

echo "Done!"
