cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "clang-18")
set(CMAKE_CXX_COMPILER "clang++-18")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
project(MyKaleidoscope)

find_package(LLVM REQUIRED CONFIG)
message("====================LLVM=========================")
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message("LLVM_INCLUDE_DIRS @ ${LLVM_INCLUDE_DIRS}")
include_directories(${LLVM_INCLUDE_DIRS})
message("LLVM_DEFINITIONS @ ${LLVM_DEFINITIONS}")
add_definitions(${LLVM_DEFINITIONS})

# include_directories("/usr/include/llvm-18")
include_directories("${CMAKE_SOURCE_DIR}/include")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(include)
add_subdirectory(src)

# include_directories(include)
# set(SOURCES
#     src/main.cpp
#     src/mylib.cpp
# )
# add_executable(MyProject ${SOURCES})
