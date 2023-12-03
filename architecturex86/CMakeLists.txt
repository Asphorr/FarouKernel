cmake_minimum_required(VERSION 3.10)

# Set the project name
project(arch)

# Add the executable
add_executable(arch main.cpp arch.cpp)

# Add the include directory
include_directories(.)

# Link the iostream library
target_link_libraries(arch stdc++)
