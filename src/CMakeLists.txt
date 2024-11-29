add_executable(parser_test lexer.cpp parser.cpp)
target_include_directories(parser_test PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(parser_test PRIVATE 
    LLVMCore
    # LLVMSupport
)
# include_directories ("${PROJECT_SOURCE_DIR}/include")

# add_executable(lexer_test lexer.cpp)
# target_include_directories(lexer_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
# include_directories ("${PROJECT_SOURCE_DIR}/include")
# include_directories(${LLVM_INCLUDE_DIRS})

# add_executable(linking_test linking_test.cpp)
# message("LLVM_LIBRARIES @ ${LLVM_LIBRARIES}")
# target_link_libraries(linking_test PRIVATE 
#     LLVMCore
#     # LLVMSupport
# )
# target_include_directories(linking_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

# add_subdirectory(lexer)
# aux_source_directory (. SRC_LIST)
# add_executable (main ${SRC_LIST})
# set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)