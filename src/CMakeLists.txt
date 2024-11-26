# add_executable(lexer_test lexer.cpp)

find_package(LLVM REQUIRED CONFIG)
message("================================================")
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
add_definitions(${LLVM_DEFINITIONS})

add_executable(parser_test lexer.cpp parser.cpp)
include_directories ("${CMAKE_SOURCE_DIR}/include")
include_directories(${LLVM_INCLUDE_DIRS})
target_link_libraries(parser_test PRIVATE ${LLVM_LIBRARIES})


# add_subdirectory(lexer)
# aux_source_directory (. SRC_LIST)
# add_executable (main ${SRC_LIST})
# set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)