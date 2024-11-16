cmake_minimum_required(VERSION 3.10)

project(MyKaleidoscope)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/bin")


add_subdirectory(src ${CMAKE_BINARY_DIR})


# include_directories(include)
# set(SOURCES
#     src/main.cpp
#     src/mylib.cpp
# )
# add_executable(MyProject ${SOURCES})