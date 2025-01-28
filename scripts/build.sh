#!/usr/bin/env bash

# Project configuration
PROJECT="funicod"
ARCH="x86_64"
SOURCES=("main.c" "utils.c")
CPP_SOURCES=("main.cpp" "utils.cpp")

# Compiler settings
export CC="gcc"
export CXX="g++"
export LD="ld.gold"  # Using gold linker by default

# Base flags for optimization
BASE_FLAGS="-Wall -O3 -march=$ARCH -mtune=$ARCH -pipe -flto -fPIE \
           -funroll-loops -finline-functions -ffunction-sections \
           -fdata-sections -fwhole-program"

export CFLAGS="$BASE_FLAGS"
export CXXFLAGS="$BASE_FLAGS -std=c++17"
export LDFLAGS="-lpthread -lrt -lm"

# Build function
build() {
    echo "Building $PROJECT..."
    
    # Compile C sources
    $CC $CFLAGS "${SOURCES[@]}" -c
    
    # Compile C++ sources and link
    $CXX $CXXFLAGS "${CPP_SOURCES[@]}" *.o $LDFLAGS -o "$PROJECT"
    
    # Optimize binary
    strip --strip-all "$PROJECT"
    
    # Compress binary (if UPX is installed)
    command -v upx >/dev/null 2>&1 && upx -9 "$PROJECT"
}

# Package function
package() {
    echo "Packaging $PROJECT..."
    tar czf "$PROJECT.tgz" "$PROJECT" README.md LICENSE 2>/dev/null
}

# Main execution
build
package

# Run the program
./"$PROJECT"
