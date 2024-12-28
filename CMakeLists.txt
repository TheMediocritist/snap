# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Project name and settings
project(snap VERSION 1.0 LANGUAGES C)

# Use C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories for header files
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find all source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.c")

# Add the executable
add_executable(snap ${SOURCES})

# Link required libraries
find_library(LIBBSD bsd REQUIRED)
target_link_libraries(snap ${LIBBSD})

# Optionally, print some diagnostic info
message(STATUS "Source files: ${SOURCES}")
message(STATUS "BSD Library: ${LIBBSD}")

