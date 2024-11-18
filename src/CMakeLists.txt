# add_executable(lexer_test lexer.cpp)
add_executable(parser_test lexer.cpp parser.cpp)
include_directories ("${CMAKE_SOURCE_DIR}/include")

# add_subdirectory(lexer)
# aux_source_directory (. SRC_LIST)
# add_executable (main ${SRC_LIST})
# set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)